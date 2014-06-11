<?php
/**
 * Created by PhpStorm.
 * User: berdir
 * Date: 4/4/14
 * Time: 4:27 PM
 */
namespace Drupal\ultimate_cron\Plugin\ultimate_cron\Logger;

use Database;
use Drupal\ultimate_cron\DatabaseLogEntry;
use Drupal\ultimate_cron\LoggerBase;
use PDO;

/**
 * Database logger.
 *
 * @LoggerPlugin(
 *   id = "database",
 *   title = @Translation("Database"),
 *   description = @Translation("Stores logs in the database."),
 * )
 */
class DatabaseLogger extends LoggerBase {
  public $options = array();
  public $log_entry_class = 'UltimateCronDatabaseLogEntry';

  const CLEANUP_METHOD_DISABLED = 1;
  const CLEANUP_METHOD_EXPIRE = 2;
  const CLEANUP_METHOD_RETAIN = 3;

  /**
   * Constructor.
   */
  public function __construct($name, $plugin, $log_type = ULTIMATE_CRON_LOG_TYPE_NORMAL) {
    parent::__construct($name, $plugin, $log_type);
    $this->options['method'] = array(
      static::CLEANUP_METHOD_DISABLED => t('Disabled'),
      static::CLEANUP_METHOD_EXPIRE => t('Remove logs older than a specified age'),
      static::CLEANUP_METHOD_RETAIN => t('Retain only a specific amount of log entries'),
    );
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'method' => static::CLEANUP_METHOD_RETAIN,
      'expire' => 86400 * 14,
      'retain' => 1000,
    );
  }

  /**
   * Cleanup logs.
   */
  public function cleanup() {
    $jobs = ultimate_cron_job_load_all();
    $current = 1;
    $max = 0;
    foreach ($jobs as $job) {
      if ($job->getPlugin($this->type)->name === $this->name) {
        $max++;
      }
    }
    foreach ($jobs as $job) {
      if ($job->getPlugin($this->type)->name === $this->name) {
        $this->cleanupJob($job);
        $class = _ultimate_cron_get_class('job');
        if ($class::$currentJob) {
          $class::$currentJob->setProgress($current / $max);
          $current++;
        }
      }
    }
  }

  /**
   * Cleanup logs for a single job.
   */
  public function cleanupJob($job) {
    $settings = $job->getSettings('logger');

    switch ($settings['method']) {
      case static::CLEANUP_METHOD_DISABLED:
        return;

      case static::CLEANUP_METHOD_EXPIRE:
        $expire = $settings['expire'];
        // Let's not delete more than ONE BILLION log entries :-o.
        $max = 10000000000;
        $chunk = 100;
        break;

      case static::CLEANUP_METHOD_RETAIN:
        $expire = 0;
        $max = db_query("SELECT COUNT(lid) FROM {ultimate_cron_log} WHERE name = :name", array(
          ':name' => $job->name,
        ))->fetchField();
        $max -= $settings['retain'];
        if ($max <= 0) {
          return;
        }
        $chunk = min($max, 100);
        break;

      default:
        watchdog('ultimate_cron', 'Invalid cleanup method: @method', array(
          '@method' => $settings['method'],
        ));
        return;
    }

    // Chunked delete.
    $count = 0;
    do {
      $lids = db_select('ultimate_cron_log', 'l')
        ->fields('l', array('lid'))
        ->condition('l.name', $job->name)
        ->condition('l.start_time', microtime(TRUE) - $expire, '<')
        ->range(0, $chunk)
        ->orderBy('l.start_time', 'ASC')
        ->orderBy('l.end_time', 'ASC')
        ->execute()
        ->fetchAll(PDO::FETCH_COLUMN);
      if ($lids) {
        $count += count($lids);
        $max -= count($lids);
        $chunk = min($max, 100);
        db_delete('ultimate_cron_log')
          ->condition('lid', $lids, 'IN')
          ->execute();
      }
    } while ($lids && $max > 0);
    if ($count) {
      watchdog('database_logger', '@count log entries removed for job @name', array(
        '@count' => $count,
        '@name' => $job->name,
      ), WATCHDOG_INFO);
    }
  }

  /**
   * Label for setting.
   */
  public function settingsLabel($name, $value) {
    switch ($name) {
      case 'method':
        return $this->options[$name][$value];
    }
    return parent::settingsLabel($name, $value);

  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = & $form['settings'][$this->type][$this->name];
    $defaults = & $form_state['default_values']['settings'][$this->type][$this->name];
    $values = & $form_state['values']['settings'][$this->type][$this->name];

    $elements['method'] = array(
      '#type' => 'select',
      '#title' => t('Log entry cleanup method'),
      '#description' => t('Select which method to use for cleaning up logs.'),
      '#options' => $this->options['method'],
      '#default_value' => $values['method'],
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $states = array('expire' => array(), 'retain' => array());

    if ($job) {
      $states['expire'] = array(
        '#states' => array(
          'visible' => array(
            ':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]' => array(
              'value' => static::CLEANUP_METHOD_EXPIRE,
            ),
          ),
          'enabled' => array(
            ':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]' => array(
              'value' => static::CLEANUP_METHOD_EXPIRE,
            ),
          )
        ),
      );

      $states['retain'] = array(
        '#states' => array(
          'visible' => array(
            ':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]' => array(
              'value' => static::CLEANUP_METHOD_RETAIN,
            ),
          ),
          'enabled' => array(
            ':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]' => array(
              'value' => static::CLEANUP_METHOD_RETAIN,
            ),
          ),
        ),
      );
    }

    $elements['method_expire'] = array(
        '#type' => 'fieldset',
        '#title' => t('Remove logs older than a specified age'),
      ) + $states['expire'];
    $elements['method_expire']['expire'] = array(
        '#parents' => array('settings', $this->type, $this->name, 'expire'),
        '#type' => 'textfield',
        '#title' => t('Log entry expiration'),
        '#description' => t('Remove log entries older than X seconds.'),
        '#default_value' => $values['expire'],
        '#fallback' => TRUE,
        '#required' => TRUE,
      ) + $states['expire'];

    $elements['method_retain'] = array(
        '#type' => 'fieldset',
        '#title' => t('Retain only a specific amount of log entries'),
      ) + $states['retain'];
    $elements['method_retain']['retain'] = array(
        '#parents' => array('settings', $this->type, $this->name, 'retain'),
        '#type' => 'textfield',
        '#title' => t('Retain logs'),
        '#description' => t('Retain X amount of log entries.'),
        '#default_value' => $values['retain'],
        '#fallback' => TRUE,
        '#required' => TRUE,
      ) + $states['retain'];

    if ($job) {
      if ($defaults['method'] == static::CLEANUP_METHOD_EXPIRE) {
        $elements['method_default'] = $elements['method_expire'];
        $elements['method_default']['#states']['visible'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
        $elements['method_default']['#states']['enabled'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
        $elements['method_default']['expire']['#states']['visible'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
        $elements['method_default']['expire']['#states']['enabled'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
      }
      if ($defaults['method'] == static::CLEANUP_METHOD_RETAIN) {
        $elements['method_default'] = $elements['method_retain'];
        $elements['method_default']['#states']['visible'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
        $elements['method_default']['#states']['enabled'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
        $elements['method_default']['retain']['#states']['visible'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
        $elements['method_default']['retain']['#states']['enabled'][':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]']['value'] = '';
      }
    }
  }

  /**
   * Submit handler.
   */
  public function settingsFormSubmit(&$form, &$form_state, $job = NULL) {
    $values = & $form_state['values']['settings'][$this->type][$this->name];
    $defaults = & $form_state['default_values']['settings'][$this->type][$this->name];
    if (!$job) {
      return;
    }

    $method = $values['method'] ? $values['method'] : $defaults['method'];

    // Cleanup form (can this be done elsewhere?)
    switch ($method) {
      case static::CLEANUP_METHOD_DISABLED:
        unset($values['expire']);
        unset($values['retain']);
        break;

      case static::CLEANUP_METHOD_EXPIRE:
        unset($values['retain']);
        break;

      case static::CLEANUP_METHOD_RETAIN:
        unset($values['expire']);
        break;
    }
  }

  /**
   * Load log entry.
   */
  public function load($name, $lock_id = NULL, $log_types = array(ULTIMATE_CRON_LOG_TYPE_NORMAL)) {
    if ($lock_id) {
      $log_entry = db_select('ultimate_cron_log', 'l')
        ->fields('l')
        ->condition('l.lid', $lock_id)
        ->execute()
        ->fetchObject('UltimateCronDatabaseLogEntry', array($name, $this));
    }
    else {
      $log_entry = db_select('ultimate_cron_log', 'l')
        ->fields('l')
        ->condition('l.name', $name)
        ->condition('l.log_type', $log_types, 'IN')
        ->orderBy('l.start_time', 'DESC')
        ->orderBy('l.end_time', 'DESC')
        ->range(0, 1)
        ->execute()
        ->fetchObject('UltimateCronDatabaseLogEntry', array($name, $this));
    }
    if ($log_entry) {
      $log_entry->finished = TRUE;
    }
    else {
      $log_entry = new DatabaseLogEntry($name, $this);
    }
    return $log_entry;
  }

  /**
   * Load latest log entry.
   */
  public function loadLatestLogEntries($jobs, $log_types) {
    if (Database::getConnection()->databaseType() !== 'mysql') {
      return parent::loadLatestLogEntries($jobs, $log_types);
    }

    $result = db_query("SELECT l.*
    FROM {ultimate_cron_log} l
    JOIN (
      SELECT l3.name, (
        SELECT l4.lid
        FROM {ultimate_cron_log} l4
        WHERE l4.name = l3.name
        AND l4.log_type IN (:log_types)
        ORDER BY l4.name desc, l4.start_time DESC
        LIMIT 1
      ) AS lid FROM {ultimate_cron_log} l3
      GROUP BY l3.name
    ) l2 on l2.lid = l.lid", array(':log_types' => $log_types));

    $log_entries = array();
    while ($object = $result->fetchObject()) {
      if (isset($jobs[$object->name])) {
        $log_entries[$object->name] = new $this->log_entry_class($object->name, $this);
        $log_entries[$object->name]->setData((array) $object);
      }
    }
    foreach ($jobs as $name => $job) {
      if (!isset($log_entries[$name])) {
        $log_entries[$name] = new $this->log_entry_class($name, $this);
      }
    }

    return $log_entries;
  }

  /**
   * Get log entries.
   */
  public function getLogEntries($name, $log_types, $limit = 10) {
    $result = db_select('ultimate_cron_log', 'l')
      ->fields('l')
      ->extend('PagerDefault')
      ->condition('l.name', $name)
      ->condition('l.log_type', $log_types, 'IN')
      ->limit($limit)
      ->orderBy('l.start_time', 'DESC')
      ->execute();

    $log_entries = array();
    while ($object = $result->fetchObject($this->log_entry_class, array(
      $name,
      $this
    ))) {
      $log_entries[$object->lid] = $object;
    }

    return $log_entries;
  }

}
