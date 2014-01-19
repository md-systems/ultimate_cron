<?php
/**
 * @file
 * Database logger for Ultimate Cron.
 */

define('ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_DISABLED', 1);
define('ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_EXPIRE', 2);
define('ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_RETAIN', 3);

class UltimateCronDatabaseLogger extends UltimateCronLogger {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'method' => ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_RETAIN,
      'expire' => 86400 * 14,
      'retain' => 1000,
    );
  }

  /**
   * Cleanup logs.
   */
  public function cleanup($job) {
    $settings = $job->getSettings('logger');

    switch ($settings['method']) {
      case ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_DISABLED:
        return;

      case ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_EXPIRE:
        $expire = $settings['expire'];
        // Let's not delete more than ONE BILLION log entries :-o.
        $max = 10000000000;
        $chunk = 100;
        break;

      case ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_RETAIN:
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
   * Save log entry.
   */
  public function save() {
    if (!$this->log_entry->lid) {
      return;
    }
    try {
      db_insert('ultimate_cron_log')
        ->fields(array(
          'lid' => $this->log_entry->lid,
          'name' => $this->log_entry->name,
          'start_time' => $this->log_entry->start_time,
          'end_time' => $this->log_entry->end_time,
          'uid' => $this->log_entry->uid,
          'init_message' => $this->log_entry->init_message,
          'message' => $this->log_entry->message,
          'severity' => $this->log_entry->severity
        ))
        ->execute();
    }
    catch (PDOException $e) {
      // Row already exists. Let's update it, if we can.
      $updated = db_update('ultimate_cron_log')
        ->fields(array(
          'name' => $this->log_entry->name,
          'start_time' => $this->log_entry->start_time,
          'end_time' => $this->log_entry->end_time,
          'init_message' => $this->log_entry->init_message,
          'message' => $this->log_entry->message,
          'severity' => $this->log_entry->severity
        ))
        ->condition('lid', $this->log_entry->lid)
        ->condition('end_time', 0)
        ->execute();
      if (!$updated) {
        // Row was not updated, someone must have beaten us to it.
        // Let's create a new log entry.
        $lid = $this->log_entry->lid . '-' . uniqid('', TRUE);
        $this->log_entry->message = t('Lock #@original_lid was already closed and logged. Creating a new log entry #@lid', array(
          '@original_lid' => $this->log_entry->lid,
          '@lid' => $lid,
        )) . "\n" . $this->log_entry->message;
        $this->log_entry->severity = $this->log_entry->severity >= 0 && $this->log_entry->severity < WATCHDOG_ERROR ? $this->log_entry->severity : WATCHDOG_ERROR;
        $this->log_entry->lid = $lid;
        $this->save();
      }
    }
  }

  /**
   * Load log entry.
   */
  public function load($job, $lid) {
    $log_entry = db_select('ultimate_cron_log', 'l')
      ->fields('l')
      ->condition('l.lid', $lid)
      ->execute()
      ->fetchObject();
    if ($log_entry) {
      $this->log_entry = $log_entry;
    }
    else {
      global $user;
      $this->log_entry->lid = NULL;
      $this->log_entry->uid = $user->uid;
      $this->log_entry->start_time = 0;
      $this->log_entry->end_time = 0;
      $this->log_entry->init_message = '';
      $this->log_entry->message = '';
      $this->log_entry->severity = -1;
    }
    $this->log_entry->module = $job->hook['module'];
    return $this;
  }

  /**
   * Load latest log entry.
   */
  public function loadLatest($job) {
    $log_entry = db_select('ultimate_cron_log', 'l')
      ->fields('l')
      ->condition('l.name', $job->name)
      ->orderBy('l.start_time', 'DESC')
      ->orderBy('l.end_time', 'DESC')
      ->range(0, 1)
      ->execute()
      ->fetchObject();
    if ($log_entry) {
      $this->log_entry = $log_entry;
    }
    else {
      $this->log_entry->lid = NULL;
      $this->log_entry->start_time = 0;
      $this->log_entry->end_time = 0;
      $this->log_entry->init_message = '';
      $this->log_entry->message = '';
      $this->log_entry->severity = -1;
      $this->log_entry->name = $job->name;
    }
    $this->log_entry->module = $job->hook['module'];
    $this->finished = TRUE;
    return $this;
  }

  /**
   * Get log entries.
   */
  public function getLogEntries($job, $limit = 10) {
    $logs = db_select('ultimate_cron_log', 'l')
      ->fields('l')
      ->extend('PagerDefault')
      ->condition('l.name', $job->name)
      ->limit($limit)
      ->orderBy('l.start_time', 'DESC')
      ->execute()
      ->fetchAll();
    return $logs;
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['method'] = array(
      '#type' => 'radios',
      '#title' => t('Log entry cleanup method'),
      '#description' => t('Select which method to use for cleaning up logs'),
      '#options' => array(
        ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_DISABLED => t('Disabled'),
        ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_EXPIRE => t('Remove logs older than a specified age'),
        ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_RETAIN => t('Retain only a specific amount of log entries'),
      ),
      '#default_value' => $values['method'],
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $elements['expire'] = array(
      '#type' => 'textfield',
      '#title' => t('Log entry expiration'),
      '#description' => t('Remove log entries older than X seconds'),
      '#default_value' => $values['expire'],
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#states' => array(
        'visible' => array(
          ':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]' => array(
            'value' => ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_EXPIRE,
          ),
        )
      ),
    );

    $elements['retain'] = array(
      '#type' => 'textfield',
      '#title' => t('Retain logs'),
      '#description' => t('Retain X amount of log entries'),
      '#default_value' => $values['retain'],
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#states' => array(
        'visible' => array(
          ':input[name="settings[' . $this->type . '][' . $this->name . '][method]"]' => array(
            'value' => ULTIMATE_CRON_DATABASE_LOGGER_CLEANUP_METHOD_RETAIN,
          ),
        )
      ),
    );

  }
}
