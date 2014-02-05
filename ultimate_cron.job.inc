<?php
/**
 * @file
 * Job class for Ultimate Cron.
 */

/**
 * Class for handling cron jobs.
 *
 * This class represents the jobs available in the system.
 */
class UltimateCronJob {
  static public $signals;
  static public $currentJob;
  public $progressUpdated = 0;
  private $plugins = array();
  private $pluginSettings = array();
  private $cacheSettings = array();

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
   * @see ultimate_cron_peek_signal()
   */
  public function peekSignal($signal) {
    if (isset(self::$signals[$this->name][$signal])) {
      return TRUE;
    }
    return ultimate_cron_peek_signal($this->name, $signal);
  }

  /**
   * Get a signal and clear it if found.
   *
   * @see ultimate_cron_get_signal()
   */
  public function getSignal($signal) {
    if (isset(self::$signals[$this->name][$signal])) {
      unset(self::$signals[$this->name][$signal]);
      return TRUE;
    }

    return ultimate_cron_get_signal($this->name, $signal);
  }

  /**
   * Send a signal.
   *
   * @see ultimate_cron_set_signal()
   */
  public function sendSignal($signal, $persist = FALSE) {
    if ($persist) {
      ultimate_cron_set_signal($this->name, $signal);
    }
    else {
      self::$signals[$this->name][$signal] = TRUE;
    }
  }

  /**
   * Clear a signal.
   *
   * @see ultimate_cron_clear_signal()
   */
  public function clearSignal($signal) {
    unset(self::$signals[$this->name][$signal]);
    ultimate_cron_clear_signal($this->name, $signal);
  }

  /**
   * Send all signal for the job.
   *
   * @see ultimate_cron_clear_signals()
   */
  public function clearSignals() {
    unset(self::$signals[$this->name]);
    ultimate_cron_clear_signals($this->name);
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
    if (isset($this->cacheSettings[$type])) {
      return $this->cacheSettings[$type];
    }
    ctools_include('plugins');
    $settings = array();

    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $plugin_info) {
      $settings[$plugin_info['type']] = $this->getPluginSettings($plugin_type);
    }
    if ($type) {
      $class = $plugin_types['ultimate_cron'][$type]['defaults']['static']['class'];
      if (!$class::$multiple) {
        $result = $settings[$type][$settings[$type]['name']];
      }
      else {
        $result = $settings[$type];
      }
    }
    else {
      $result = $settings;
    }

    $this->cacheSettings[$type] = $result;
    return $result;
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
    if (isset($this->plugins[$plugin_type])) {
      return $this->plugins[$plugin_type];
    }

    if ($name) {
    }
    elseif (!empty($this->settings[$plugin_type]['name'])) {
      $name = $this->settings[$plugin_type]['name'];
    }
    else {
      $name = $this->hook[$plugin_type]['name'];
    }
    $this->plugins[$plugin_type] = ultimate_cron_plugin_load($plugin_type, $name);
    return $this->plugins[$plugin_type];
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

    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    $plugin_info = $plugin_types['ultimate_cron'][$plugin_type];
    $static = $plugin_info['defaults']['static'];
    $class = $static['class'];

    $settings = $this->settings[$plugin_type];

    if (!$class::$multiple) {
      $plugin = $this->getPlugin($plugin_type);
      if (empty($settings[$plugin->name])) {
        $settings[$plugin->name] = array();
      }
      $settings['name'] = $plugin->name;
      $settings[$plugin->name] += $plugin->getDefaultSettings($this);
    }
    else {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $name => $plugin) {
        if (empty($settings[$name])) {
          $settings[$name] = array();
        }
        if ($plugin->isValid($this)) {
          $settings[$name] += $plugin->getDefaultSettings($this);
        }
      }
    }
    $this->pluginSettings[$plugin_type] = $settings;
    return $settings;
  }

  /**
   * Log kill signal recieved.
   */
  public function kill() {
    watchdog('ultimate_cron', 'kill signal recieved', array(), WATCHDOG_WARNING);
  }

  /**
   * Enable a job.
   */
  public function enable() {
    if (empty($this->hook['immutable'])) {
      ctools_include('export');
      ctools_export_crud_enable('ultimate_cron_job', $this);
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Disable a job.
   */
  public function disable() {
    if (empty($this->hook['immutable'])) {
      ctools_include('export');
      ctools_export_crud_disable('ultimate_cron_job', $this);
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Custom action for plugins.
   */
  public function custom_page($js, $input, $item, $plugin_type, $plugin_name, $action) {
    $plugin = ultimate_cron_plugin_load($plugin_type, $plugin_name);
    return $plugin->custom_page($js, $input, $item, $action);
  }

  /**
   * Allow a job to alter the allowed operations on it in the Export UI.
   */
  public function build_operations_alter(&$allowed_operations) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $name => $info) {
      $static = $info['defaults']['static'];
      $class = $static['class'];
      if (!$class::$multiple) {
        $this->getPlugin($name)->build_operations_alter($this, $allowed_operations);
      }
      else {
        $plugins = ultimate_cron_plugin_load_all($name);
        foreach ($plugins as $plugin) {
          $this->getPlugin($name, $plugin->name)->build_operations_alter($this, $allowed_operations);
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
      UltimateCronPlugin::hook_cron_pre_invoke($this);
      module_invoke_all('cron_pre_invoke', $this);
      switch ($this->hook['api_version']) {
        case 'ultimate_cron-1':
          // error_log("$this->name : INVOKE 1.x");
          if (is_callable($this->hook['callback'])) {
            $result = call_user_func_array($this->hook['callback'], array(
              $this->name, $this->hook
            ));
            break;
          }
          else {
            $result = module_invoke($this->hook['module'], 'cronapi', 'execute', $this->name, $this->hook);
            break;
          }

        case 'ultimate_cron-2':
          // error_log("$this->name : INVOKE 2.x");
          if (isset($this->hook['file'])) {
            require_once $this->hook['file path'] . '/' . $this->hook['file'];
          }
          $result = call_user_func_array($this->hook['callback'], array(
            $this, $this->hook['callback arguments']
          ));
          break;

        case 'elysia_cron-2':
          if (isset($this->hook['file'])) {
            require_once $this->hook['file path'] . '/' . $this->hook['file'];
          }
          // error_log("$this->name : INVOKE ELYSIA CRON 2.x");
          if (is_callable($this->hook['callback'])) {
            $arguments = !empty($this->hook['arguments']) ? $this->hook['arguments'] : array();
            $result = call_user_func_array($this->hook['callback'], $arguments);
            break;
          }
          else {
            $result = module_invoke($this->hook['module'], 'cronapi', 'execute', $this->name);
            break;
          }

        default:
          // error_log("$this->name : COULD NOT INVOKE JOB");
          throw new Exception(t('Could not invoke cron job @name. Wrong API version (@api_version)', array(
            '@name' => $this->name,
            '@api_version' => $this->hook['api_version'],
          )));
      }
    }
    catch (Exception $e) {
      UltimateCronPlugin::hook_cron_post_invoke($this);
      module_invoke_all('cron_post_invoke', $this);
      throw $e;
    }

    UltimateCronPlugin::hook_cron_post_invoke($this);
    module_invoke_all('cron_post_invoke', $this);
    return $result;
  }

  /**
   * Check job schedule.
   */
  public function isScheduled() {
    UltimateCronPlugin::hook_cron_pre_schedule($this);
    module_invoke_all('cron_pre_schedule', $this);
    $result = empty($this->disabled) && !$this->isLocked() && $this->getPlugin('scheduler')->isScheduled($this);
    UltimateCronPlugin::hook_cron_post_schedule($this, $result);
    module_invoke_all('cron_post_schedule', $this, $result);
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
    UltimateCronPlugin::hook_cron_pre_launch($this);
    module_invoke_all('cron_pre_launch', $this);
    $result = $this->getPlugin('launcher')->launch($this);
    UltimateCronPlugin::hook_cron_post_launch($this);
    module_invoke_all('cron_post_launch', $this);
    return $result;
  }

  /**
   * Lock job.
   */
  public function lock() {
    $launcher = $this->getPlugin('launcher');
    $lock_id = $launcher->lock($this);
    if (!$lock_id) {
      watchdog('ultimate_cron', 'Could not get lock for job @name', array(
        '@name' => $this->name,
      ), WATCHDOG_ERROR);
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
      $launchers[$job->getPlugin('launcher')->name][$job->name] = $job;
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
    $this->setProgress(FALSE);
    UltimateCronPlugin::hook_cron_pre_run($this);
    module_invoke_all('cron_pre_run', $this);
    self::$currentJob = $this;
    $result = $this->getPlugin('launcher')->run($this);
    self::$currentJob = NULL;
    UltimateCronPlugin::hook_cron_post_run($this);
    module_invoke_all('cron_post_run', $this);
    $this->setProgress(FALSE);
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
  public function getLogEntries($limit = 10) {
    return $this->getPlugin('logger')->getLogEntries($this->name, $limit);
  }

  /**
   * Load log entry.
   *
   * @param string $lock_id
   *   The lock id of the log entry.
   *
   * @return UltimateCronLogEntry
   *   The log entry.
   */
  public function loadLogEntry($lock_id) {
    return $this->getPlugin('logger')->load($this->name, $lock_id);
  }

  /**
   * Load latest log.
   *
   * @return UltimateCronLogEntry
   *   The latest log entry for this job.
   */
  public function loadLatestLogEntry() {
    return $this->getPlugin('logger')->load($this->name);
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
  static public function loadLatestLogEntries($jobs) {
    $loggers = array();
    foreach ($jobs as $job) {
      $loggers[$job->getPlugin('logger')->name][$job->name] = $job;
    }
    $log_entries = array();
    foreach ($loggers as $logger => $jobs) {
      $log_entries += ultimate_cron_plugin_load('logger', $logger)->loadLatestLogEntries($jobs);
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
   * @return UltimateCronLogger
   *   The log object.
   */
  public function startLog($lock_id, $init_message = '') {
    $logger = $this->getPlugin('logger');
    $log_entry = $logger->create($this->name, $lock_id, $init_message);
    $logger->catchMessages($log_entry);
    return $log_entry;
  }

  /**
   * Resume a previosly saved log.
   *
   * @param string $lock_id
   *   The lock id of the log to resume.
   *
   * @return UltimateCronLogEntry
   *   The log entry object.
   */
  public function resumeLog($lock_id) {
    $logger = $this->getPlugin('logger');
    $log_entry = $logger->load($this->name, $lock_id);
    $log_entry->finished = FALSE;
    $logger->catchMessages($log_entry);
    return $log_entry;
  }

  /**
   * Get module name for this job.
   */
  public function getModuleName() {
    $info = system_get_info('module', $this->hook['module']);
    return $info && !empty($info['name']) ? $info['name'] : $this->hook['module'];
  }

  /**
   * Get module description for this job.
   */
  public function getModuleDescription() {
    $info = system_get_info('module', $this->hook['module']);
    return $info && !empty($info['description']) ? $info['description'] : '';
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
      $launchers[$job->getPlugin('launcher')->name][$job->name] = $job;
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
   * Send a nodejs message.
   *
   * @param string $action
   *   The action performed.
   * @param array $data
   *   Data blob for the given action.
   */
  public function sendMessage($action, $data = array()) {
    if (module_exists('nodejs')) {
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
   * Rebuild a row on the export ui.
   */
  public function rebuild_ctools_export_ui_table_row() {
    ctools_include('export-ui');
    $plugin = ctools_get_export_ui('ultimate_cron_job_ctools_export_ui');
    $handler = ctools_export_ui_get_handler($plugin);
    $operations = $handler->build_operations($this);
    $form_state = array();
    $form_state['values']['order'] = '';
    $handler->list_build_row($this, $form_state, $operations);
    $row = $handler->rows[$this->name];
    $cells = isset($row['data']) ? $row['data'] : $row;
    $final_cells = array();
    foreach ($cells as $cell) {
      $data = _theme_table_cell($cell);
      $final_cells[] = $data;
    }
    return $final_cells;
  }
}
