<?php
/**
 * @file
 * Job class for Ultimate Cron.
 */

namespace Drupal\ultimate_cron\Entity;

use Drupal\Core\Config\Entity\ConfigEntityBase;
use Drupal\Core\Entity\EntityStorageInterface;
use Drupal\Core\Logger\RfcLogLevel;
use Drupal\ultimate_cron\CronJobHelper;
use Drupal\ultimate_cron\CronJobInterface;
use Drupal\ultimate_cron\CronPlugin;
use Drupal\ultimate_cron\Logger\LogEntry;
use Drupal\ultimate_cron\Logger\LoggerBase;
use Exception;

/**
 * Class for handling cron jobs.
 *
 * This class represents the jobs available in the system.
 *
 * @ConfigEntityType(
 *   id = "ultimate_cron_job",
 *   label = @Translation("Cron Job"),
 *   handlers = {
 *     "list_builder" = "Drupal\ultimate_cron\CronJobListBuilder",
 *     "form" = {
 *       "default" = "Drupal\ultimate_cron\Form\CronJobForm",
 *       "delete" = "\Drupal\Core\Entity\EntityDeleteForm",
 *     }
 *   },
 *   config_prefix = "job",
 *   admin_permission = "administer ultimate cron",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "title",
 *     "status" = "status",
 *   },
 *   links = {
 *     "edit-form" = "/admin/config/system/cron/jobs/manage/{ultimate_cron_job}",
 *     "delete-form" = "/admin/config/system/cron/jobs/manage/{ultimate_cron_job}/delete",
 *     "collection" = "/admin/config/system/cron/jobs",
 *     "run" = "/admin/config/system/cron/jobs/{ultimate_cron_job}/run",
 *   }
 * )
 *
 */
class CronJob extends ConfigEntityBase implements CronJobInterface {
  static public $signals;
  static public $currentJob;
  public $progressUpdated = 0;
  public $settings;

  /**
   * @var int
   */
  protected $id;

  /**
   * @var int
   */
  protected $uuid;

  /**
   * @var bool
   */
  protected $status = TRUE;

  /**
   * @var string
   */
  protected $title;

  /**
   * @var string
   */
  protected $callback;

  /**
   * @var string
   */
  protected $module;

  /**
   * @var array
   */
  protected $scheduler = array('id' => 'simple');

  /**
   * @var array
   */
  protected $launcher = array('id' => 'serial');

  /**
   * @var array
   */
  protected $logger = array('id' => 'database');

  public function postSave(EntityStorageInterface $storage, $update = TRUE) {
    parent::postSave($storage, $update);
    if ($update && empty($this->dont_log)) {
      $log = $this->startLog(uniqid($this->id(), TRUE), '', ULTIMATE_CRON_LOG_TYPE_ADMIN);
      $log->log($this->id(), 'Job modified by ' . $log->formatUser(), array(), RfcLogLevel::INFO);
      $log->finish();
    }
  }

  public function setConfiguration($plugin_type, $configuration) {
    $this->{$plugin_type}['configuration'] = $configuration;
  }

  /**
   * {@inheritdoc}
   */
  public static function postDelete(EntityStorageInterface $storage, array $entities) {
    foreach ($entities as $entity) {
      if (empty($entity->dont_log)) {
        $log = $entity->startLog(uniqid($entity->id(), TRUE), 'modification', ULTIMATE_CRON_LOG_TYPE_ADMIN);
        $log->log($entity->id(), 'Job deleted by ' . $log->formatUser(), array(), RfcLogLevel::INFO);
        $log->finish();
      }
    }
  }

  /**
   * Invoke plugin cron_alter().
   *
   * Calls on cron_alter() on all valid plugins for this job.
   */
  public function cron_alter() {
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $class = $info['defaults']['static']['class'];
      if ($class::$multiple) {
        $plugins = ultimate_cron_plugin_load_all($plugin_type);
        foreach ($plugins as $plugin) {
          if ($plugin->isValid($this)) {
            $plugin->cron_alter($this);
          }
        }
      }
      else {
        $plugin = $this->getPlugin($plugin_type);
        $plugin->cron_alter($this);
      }
    }
  }

  /**
   * Get a signal without affecting it.
   *
   * @see UltimateCronSignal::peek()
   */
  public function peekSignal($signal) {
    if (isset(self::$signals[$this->id()][$signal])) {
      return TRUE;
    }
    $signal = \Drupal::service('ultimate_cron.signal');;
    return $signal->peek($this->id(), $signal);
  }

  /**
   * Get a signal and clear it if found.
   *
   * @see UltimateCronSignal::get()
   */
  public function getSignal($signal) {
    if (isset(self::$signals[$this->id()][$signal])) {
      unset(self::$signals[$this->id()][$signal]);
      return TRUE;
    }
    $signal = \Drupal::service('ultimate_cron.signal');;
    return $signal->get($this->id(), $signal);
  }

  /**
   * Send a signal.
   *
   * @see UltimateCronSignal::set()
   */
  public function sendSignal($signal, $persist = FALSE) {
    if ($persist) {
      $signal = \Drupal::service('ultimate_cron.signal');;
      $signal->set($this->id(), $signal);
    }
    else {
      self::$signals[$this->id()][$signal] = TRUE;
    }
  }

  /**
   * Clear a signal.
   *
   * @see UltimateCronSignal::clear()
   */
  public function clearSignal($signal) {
    unset(self::$signals[$this->id()][$signal]);
    $signal = \Drupal::service('ultimate_cron.signal');;
    $signal->clear($this->id(), $signal);
  }

  /**
   * Send all signal for the job.
   *
   * @see UltimateCronSignal::flush()
   */
  public function clearSignals() {
    unset(self::$signals[$this->id()]);
    $signal = \Drupal::service('ultimate_cron.signal');;
    $signal->flush($this->id());
  }

  /**
   * Get job settings.
   *
   * @param string $type
   *   (optional) The plugin type to get settings for.
   *
   * @return array
   *   The settings for the given plugin. If no plugin is given, returns
   *   all settings.
   */
  public function getSettings($type = '') {
    if (isset($this->cacheSettings)) {
      if ($type) {
        $settings = !empty($this->cacheSettings['scheduler'][$type]['name']) ? $this->cacheSettings[$type][$this->cacheSettings['scheduler'][$type]['name']] : $this->cacheSettings['scheduler'][$type];
      }
      else {
        $settings = $this->cacheSettings;
      }
      return $settings;
    }
    $settings = array();

    $plugin_types = CronJobHelper::getPluginTypes();
    foreach ($plugin_types as $plugin_type => $plugin_info) {
      $settings[$plugin_type] = $this->getPluginSettings($plugin_type);
    }

    $this->cacheSettings = $settings;
    return $this->getSettings($type);
  }

  /**
   * Get job plugin.
   *
   * If no plugin name is provided current plugin of the specified type will
   * be returned.
   *
   * @param string $plugin_type
   *   Name of plugin type.
   * @param string $name
   *   (optional) The name of the plugin.
   *
   * @return mixed
   *   Plugin instance of the specified type.
   */
  public function getPlugin($plugin_type, $name = NULL) {
    if ($name) {
      return ultimate_cron_plugin_load($plugin_type, $name);
    }
    // @todo: enable static cache, needs unset when values change.
//    if (isset($this->plugins[$plugin_type])) {
//      return $this->plugins[$plugin_type];
//    }

    if ($name) {
    }
    elseif (!empty($this->{$plugin_type}['id'])) {
      $name = $this->{$plugin_type}['id'];
    }
    else {
      $name = $this->hook[$plugin_type]['name'];
    }
    /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
    $manager = \Drupal::service('plugin.manager.ultimate_cron.' . $plugin_type);
    $this->plugins[$plugin_type] = $manager->createInstance($name, isset($this->{$plugin_type}['configuration']) ? $this->{$plugin_type}['configuration'] : array());
    return $this->plugins[$plugin_type];
  }

  public function getConfiguration($plugin_type) {
    if(!isset($this->{$plugin_type}['configuration'])) {
      $this->{$plugin_type}['configuration'] = $this->getPlugin($plugin_type)->defaultConfiguration();
    }

    return $this->{$plugin_type}['configuration'];
  }

  /**
   * Get plugin settings.
   *
   * @param string $plugin_type
   *   The plugin type.
   *
   * @return array
   *   Settings for the given plugin type.
   */
  public function getPluginSettings($plugin_type) {
    if (isset($this->pluginSettings[$plugin_type])) {
      return $this->pluginSettings[$plugin_type];
    }

//    $plugin_types = CronJobHelper::getPluginTypes();
//    $plugin_info = $plugin_types[$plugin_type];
//    $static = $plugin_info['defaults']['static'];
////    $class = $static['class'];

    $settings = $this->settings[$plugin_type];

//    if (!$class::$multiple) {
//      $plugin = $this->getPlugin($plugin_type);
//      if (empty($settings[$plugin->name])) {
//        $settings[$plugin->name] = array();
//      }
//      $settings['name'] = $plugin->name;
//      $settings[$plugin->name] += $plugin->getDefaultSettings($this);
//    }
//    else {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $name => $plugin) {
        if (empty($settings[$name])) {
          $settings[$name] = array();
        }
        if ($plugin->isValid($this)) {
          $settings[$name] += $plugin->defaultConfiguration($this);
        }
      }
//    }
    $this->pluginSettings[$plugin_type] = $settings;
    return $settings;
  }

  /**
   * Signal page for plugins.
   */
  public function signal($item, $plugin_type, $plugin_name, $signal) {
    $plugin = ultimate_cron_plugin_load($plugin_type, $plugin_name);
    return $plugin->signal($item, $signal);
  }

  /**
   * Allow a job to alter the allowed operations on it in the Export UI.
   */
  public function build_operations_alter(&$allowed_operations) {
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $name => $info) {
      $static = $info['defaults']['static'];
      $class = $static['class'];
      if (!$class::$multiple) {
        $this->getPlugin($name)
          ->build_operations_alter($this, $allowed_operations);
      }
      else {
        $plugins = ultimate_cron_plugin_load_all($name);
        foreach ($plugins as $plugin) {
          $this->getPlugin($name, $plugin->name)
            ->build_operations_alter($this, $allowed_operations);
        }
      }
    }
    drupal_alter('ultimate_cron_plugin_build_operations', $this, $allowed_operations);
  }

  /**
   * Invoke the jobs callback.
   */
  public function invoke() {
    try {
      CronPlugin::hook_cron_pre_invoke($this);
      \Drupal::moduleHandler()->invokeAll('cron_pre_invoke', array($this));

      $callback = $this->callback;
      $result = $callback($this->id());

    } catch (Exception $e) {
      CronPlugin::hook_cron_post_invoke($this);
      \Drupal::moduleHandler()->invokeAll('cron_post_invoke', array($this));
      throw $e;
    }

    CronPlugin::hook_cron_post_invoke($this);
    \Drupal::moduleHandler()->invokeAll('cron_post_invoke', array($this));
    return $result;
  }

  /**
   * Check job schedule.
   */
  public function isScheduled() {
    CronPlugin::hook_cron_pre_schedule($this);
    \Drupal::moduleHandler()->invokeAll('cron_pre_schedule', array($this));
    $result = empty($this->disabled) && !$this->isLocked() && $this->getPlugin('scheduler')
        ->isScheduled($this);
    CronPlugin::hook_cron_post_schedule($this, $result);
    \Drupal::moduleHandler()->invokeAll('cron_post_schedule', array($this));
    return $result;
  }

  /**
   * Check if job is behind its schedule.
   */
  public function isBehindSchedule() {
    return $this->getPlugin('scheduler')->isBehind($this);
  }

  /**
   * Launch job.
   */
  public function launch() {
    CronPlugin::hook_cron_pre_launch($this);
    \Drupal::moduleHandler()->invokeAll('cron_pre_launch', array($this));
    $result = $this->getPlugin('launcher')->launch($this);
    CronPlugin::hook_cron_post_launch($this);
    \Drupal::moduleHandler()->invokeAll('cron_post_launch', array($this));
    return $result;
  }

  /**
   * Lock job.
   */
  public function lock() {
    $launcher = $this->getPlugin('launcher');
    $lock_id = $launcher->lock($this);
    if (!$lock_id) {
      \Drupal::logger('ultimate_cron')->error('Could not get lock for job @name', array(
        '@name' => $this->id(),
      ));
      return FALSE;
    }
    $this->sendMessage('lock', array(
      'lock_id' => $lock_id,
    ));
    return $lock_id;
  }

  /**
   * Unlock job.
   *
   * @param string $lock_id
   *   The lock id to unlock.
   * @param boolean $manual
   *   Whether or not this is a manual unlock.
   */
  public function unlock($lock_id = NULL, $manual = FALSE) {
    $result = NULL;
    if (!$lock_id) {
      $lock_id = $this->isLocked();
    }
    if ($lock_id) {
      $result = $this->getPlugin('launcher')->unlock($lock_id, $manual);
    }
    $this->sendMessage('unlock', array(
      'lock_id' => $lock_id,
    ));
    return $result;
  }

  /**
   * Get locked state of job.
   */
  public function isLocked() {
    return $this->getPlugin('launcher')->isLocked($this);
  }

  /**
   * Get locked state for multiple jobs.
   *
   * @param array $jobs
   *   Jobs to check locks for.
   */
  static public function isLockedMultiple($jobs) {
    $launchers = array();
    foreach ($jobs as $job) {
      $launchers[$job->getPlugin('launcher')->name][$job->id()] = $job;
    }
    $locked = array();
    foreach ($launchers as $launcher => $jobs) {
      $locked += ultimate_cron_plugin_load('launcher', $launcher)->isLockedMultiple($jobs);
    }
    return $locked;
  }

  /**
   * Run job.
   */
  public function run() {
    $this->clearSignals();
    $this->initializeProgress();
    CronPlugin::hook_cron_pre_run($this);
    \Drupal::moduleHandler()->invokeAll('cron_pre_run', array($this));
    self::$currentJob = $this;
    $result = $this->getPlugin('launcher')->run($this);
    self::$currentJob = NULL;
    CronPlugin::hook_cron_post_run($this);
    \Drupal::moduleHandler()->invokeAll('cron_post_run', array($this));
    $this->finishProgress();
    return $result;
  }

  /**
   * Get log entries.
   *
   * @param integer $limit
   *   (optional) Number of log entries per page.
   *
   * @return array
   *   Array of UltimateCronLogEntry objects.
   */
  public function getLogEntries($log_types = ULTIMATE_CRON_LOG_TYPE_ALL, $limit = 10) {
    $log_types = $log_types == ULTIMATE_CRON_LOG_TYPE_ALL ? _ultimate_cron_define_log_type_all() : $log_types;
    return $this->getPlugin('logger')
      ->getLogEntries($this->id(), $log_types, $limit);
  }

  /**
   * Load log entry.
   *
   * @param string $lock_id
   *   The lock id of the log entry.
   *
   * @return LogEntry
   *   The log entry.
   */
  public function loadLogEntry($lock_id) {
    return $this->getPlugin('logger')->load($this->id(), $lock_id);
  }

  /**
   * Load latest log.
   *
   * @return LogEntry
   *   The latest log entry for this job.
   */
  public function loadLatestLogEntry($log_types = array(ULTIMATE_CRON_LOG_TYPE_NORMAL)) {
    return $this->getPlugin('logger')->load($this->id(), NULL, $log_types);
  }

  /**
   * Load latest log entries.
   *
   * @param array $jobs
   *   Jobs to load log entries for.
   *
   * @return array
   *   Array of UltimateCronLogEntry objects.
   */
  static public function loadLatestLogEntries($jobs, $log_types = array(ULTIMATE_CRON_LOG_TYPE_NORMAL)) {
    $loggers = array();
    foreach ($jobs as $job) {
      $loggers[$job->getPlugin('logger')->name][$job->id()] = $job;
    }
    $log_entries = array();
    foreach ($loggers as $logger => $jobs) {
      $log_entries += ultimate_cron_plugin_load('logger', $logger)->loadLatestLogEntries($jobs, $log_types);
    }
    return $log_entries;
  }


  /**
   * Start logging.
   *
   * @param string $lock_id
   *   The lock id to use.
   * @param string $init_message
   *   Initial message for the log.
   *
   * @return LoggerBase
   *   The log object.
   */
  public function startLog($lock_id, $init_message = '', $log_type = ULTIMATE_CRON_LOG_TYPE_NORMAL) {
    $logger = $this->getPlugin('logger');
    $log_entry = $logger->create($this->id(), $lock_id, $init_message, $log_type);
    $logger->catchMessages($log_entry);
    return $log_entry;
  }

  /**
   * Resume a previosly saved log.
   *
   * @param string $lock_id
   *   The lock id of the log to resume.
   *
   * @return LogEntry
   *   The log entry object.
   */
  public function resumeLog($lock_id) {
    $logger = $this->getPlugin('logger');
    $log_entry = $logger->load($this->id(), $lock_id);
    $log_entry->finished = FALSE;
    $logger->catchMessages($log_entry);
    return $log_entry;
  }

  /**
   * Get module name for this job.
   */
  public function getModuleName() {
    static $names = array();
    if (!isset($names[$this->module])) {
      $info = system_get_info('module', $this->module);
      $names[$this->module] = $info && !empty($info['name']) ? $info['name'] : $this->module;
    }
    return $names[$this->module];
  }

  /**
   * Get module description for this job.
   */
  public function getModuleDescription() {
    static $descs = array();
    if (!isset($descs[$this->module])) {
      $info = system_get_info('module', $this->module);
      $descs[$this->module] = $info && !empty($info['description']) ? $info['description'] : '';
    }
    return $descs[$this->module];
  }

  /**
   * Initialize progress.
   */
  public function initializeProgress() {
    return $this->getPlugin('launcher')->initializeProgress($this);
  }

  /**
   * Finish progress.
   */
  public function finishProgress() {
    return $this->getPlugin('launcher')->finishProgress($this);
  }

  /**
   * Get job progress.
   *
   * @return float
   *   The progress of this job.
   */
  public function getProgress() {
    return $this->getPlugin('launcher')->getProgress($this);
  }

  /**
   * Get multiple job progresses.
   *
   * @param array $jobs
   *   Jobs to get progress for.
   *
   * @return array
   *   Progress of jobs, keyed by job name.
   */
  static public function getProgressMultiple($jobs) {
    $launchers = array();
    foreach ($jobs as $job) {
      $launchers[$job->getPlugin('launcher')->name][$job->id()] = $job;
    }
    $progresses = array();
    foreach ($launchers as $launcher => $jobs) {
      $progresses += ultimate_cron_plugin_load('launcher', $launcher)->getProgressMultiple($jobs);
    }
    return $progresses;
  }

  /**
   * Set job progress.
   *
   * @param float $progress
   *   The progress (0 - 1).
   */
  public function setProgress($progress) {
    if ($this->getPlugin('launcher')->setProgress($this, $progress)) {
      $this->sendMessage('setProgress', array(
        'progress' => $progress,
      ));
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Format progress.
   *
   * @param float $progress
   *   (optional) The progress to format. Uses the progress on the object
   *              if not specified.
   *
   * @return string
   *   Formatted progress.
   */
  public function formatProgress($progress = NULL) {
    if (!isset($progress)) {
      $progress = isset($this->progress) ? $this->progress : $this->getProgress();
    }
    return $this->getPlugin('launcher')->formatProgress($this, $progress);
  }

  /**
   * Get a "unique" id for a job.
   */
  public function getUniqueID() {
    return isset($this->ids[$this->id()]) ? $this->ids[$this->id()] : $this->ids[$this->id()] = hexdec(substr(sha1($this->id()), -8));
  }

  /**
   * Send a nodejs message.
   *
   * @param string $action
   *   The action performed.
   * @param array $data
   *   Data blob for the given action.
   */
  public function sendMessage($action, $data = array()) {
    if (\Drupal::moduleHandler()->moduleExists('nodejs')) {
      $settings = ultimate_cron_plugin_load('settings', 'general')->getDefaultSettings();
      if (empty($settings['nodejs'])) {
        return;
      }

      $elements = array();

      $build = clone $this;

      $cell_idxs = array();

      switch ($action) {
        case 'lock':
          $logger = $build->getPlugin('logger');
          if (empty($data['log_entry'])) {
            $build->lock_id = $data['lock_id'];
            $build->log_entry = $logger->factoryLogEntry($build->name);
            $build->log_entry->setData(array(
              'lid' => $data['lock_id'],
              'start_time' => microtime(TRUE),
            ));
          }
          else {
            $build->log_entry = $data['log_entry'];
          }
          $cell_idxs = array(
            'tr#' . $build->name . ' .ctools-export-ui-start-time' => 3,
            'tr#' . $build->name . ' .ctools-export-ui-duration' => 4,
            'tr#' . $build->name . ' .ctools-export-ui-status' => 5,
            'tr#' . $build->name . ' .ctools-export-ui-operations' => 7,
          );
          break;

        case 'unlock':
          $build->log_entry = $build->loadLogEntry($data['lock_id']);
          $build->lock_id = FALSE;
          $cell_idxs = array(
            'tr#' . $build->name . ' .ctools-export-ui-start-time' => 3,
            'tr#' . $build->name . ' .ctools-export-ui-duration' => 4,
            'tr#' . $build->name . ' .ctools-export-ui-status' => 5,
            'tr#' . $build->name . ' .ctools-export-ui-operations' => 7,
          );
          break;

        case 'setProgress':
          $build->lock_id = $build->isLocked();
          $build->log_entry = $build->loadLogEntry($build->lock_id);
          $cell_idxs = array(
            'tr#' . $build->name . ' .ctools-export-ui-start-time' => 3,
            'tr#' . $build->name . ' .ctools-export-ui-duration' => 4,
            'tr#' . $build->name . ' .ctools-export-ui-status' => 5,
          );
          break;
      }
      $cells = $build->rebuild_ctools_export_ui_table_row();
      foreach ($cell_idxs as $selector => $cell_idx) {
        $elements[$selector] = $cells[$cell_idx];
      }

      $message = (object) array(
        'channel' => 'ultimate_cron',
        'data' => (object) array(
            'action' => $action,
            'job' => $build,
            'timestamp' => microtime(TRUE),
            'elements' => $elements,
          ),
        'callback' => 'nodejsUltimateCron',
      );
      nodejs_send_content_channel_message($message);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function calculateDependencies() {
    parent::calculateDependencies();

    $this->addDependency('module', $this->getModule());

    return $this->dependencies;
  }

  /**
   * Rebuild a row on the export ui.
   */
  public function rebuild_ctools_export_ui_table_row() {
    $plugin = ctools_get_export_ui('ultimate_cron_job_ctools_export_ui');
    $handler = ctools_export_ui_get_handler($plugin);
    $operations = $handler->build_operations($this);
    $form_state = array();
    $form_state['values']['order'] = '';
    $handler->list_build_row($this, $form_state, $operations);
    $row = $handler->rows[$this->id()];
    $cells = isset($row['data']) ? $row['data'] : $row;
    $final_cells = array();
    foreach ($cells as $cell) {
      $data = _theme_table_cell($cell);
      $final_cells[] = $data;
    }
    return $final_cells;
  }

  /**
   * {@inheritdoc}
   */
  public function getTitle() {
    return $this->title;
  }

  /**
   * {@inheritdoc}
   */
  public function getCallback() {
    return $this->callback;
  }

  /**
   * {@inheritdoc}
   */
  public function getModule() {
    return $this->module;
  }

  /**
   * {@inheritdoc}
   */
  public function getSchedulerId() {
    return $this->scheduler;
  }

  /**
   * {@inheritdoc}
   */
  public function getLauncherId() {
    return $this->launcher['id'];
  }

  /**
   * {@inheritdoc}
   */
  public function getLoggerId() {
    return $this->logger;
  }

  /**
   * {@inheritdoc}
   */
  public function setTitle($title) {
    $this->set('title', $title);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function setCallback($callback) {
    $this->set('callback', $callback);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function setModule($module) {
    $this->set('module', $module);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function setSchedulerId($scheduler_id) {
    $this->scheduler['id'] = $scheduler_id;
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function setLauncherId($launcher_id) {
    $this->launcher['id'] = $launcher_id;
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function setLoggerId($logger_id) {
    $this->launcher['id'] = $logger_id;
      return $this;
  }
}
