<?php
/**
 * @file
 * Job class for Ultimate Cron.
 */

class UltimateCronJob {
  static public $signals;
  static public $currentJob;

  /**
   * Invoke plugin cron_alter().
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
   */
  public function peekSignal($signal) {
    if (isset(self::$signals[$this->name][$signal])) {
      return TRUE;
    }
    return ultimate_cron_peek_signal($this->name, $signal);
  }

  /**
   * Get a signal and clear it if found.
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
   */
  public function clearSignal($signal) {
    unset(self::$signals[$this->name][$signal]);
    ultimate_cron_clear_signal($this->name, $signal);
  }

  /**
   * Send all signal for the job.
   */
  public function clearSignals() {
    unset(self::$signals[$this->name]);
    ultimate_cron_clear_signals($this->name);
  }

  /**
   * Get job settings.
   */
  public function getSettings($type = NULL) {
    ctools_include('plugins');
    $settings = array();

    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $plugin_info) {
      $settings[$plugin_info['type']] = $this->getPluginSettings($plugin_type);
    }
    if ($type) {
      $class = $plugin_types['ultimate_cron'][$type]['defaults']['static']['class'];
      if (!$class::$multiple) {
        return $settings[$type][$settings[$type]['name']];
      }
      return $settings[$type];
    }
    return $settings;
  }

  /**
   * Get job plugin.
   *
   * @param string $plugin_type
   *   Name of plugin type.
   *
   * @return mixed
   *   Plugin instance of the specified type.
   */
  public function getPlugin($plugin_type, $name = NULL) {
    if ($name) {
      return ultimate_cron_plugin_load($plugin_type, $name);
    }
    elseif (!empty($this->settings[$plugin_type]['name'])) {
      return ultimate_cron_plugin_load($plugin_type, $this->settings[$plugin_type]['name']);
    }
    else {
      return ultimate_cron_plugin_load($plugin_type, $this->hook[$plugin_type]['name']);
    }
  }

  /**
   * Get plugin settings.
   */
  public function getPluginSettings($plugin_type) {
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
    return $settings;
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
  }

  /**
   * Invoke the jobs callback.
   */
  public function invoke() {
    UltimateCronPlugin::hook_cron_pre_invoke($this);
    module_invoke_all('cron_pre_invoke', $this);
    switch ($this->hook['api_version']) {
      case 'ultimate_cron-1':
        // error_log("$this->name : INVOKE 1.x");
        $args = array($this->hook);
        $args = array_merge($args, $this->hook['callback_arguments']);
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
        $result = call_user_func_array($this->hook['callback'], array(
          $this, $this->hook['callback_arguments']
        ));
        break;

      case 'elysia_cron-2':
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
    UltimateCronPlugin::hook_cron_post_invoke($this);
    module_invoke_all('cron_post_invoke', $this);
    return $result;
  }

  /**
   * Check job schedule.
   */
  public function schedule() {
    UltimateCronPlugin::hook_cron_pre_schedule($this);
    module_invoke_all('cron_pre_schedule', $this);
    $result = empty($this->disabled) && $this->getPlugin('scheduler')->schedule($this);
    UltimateCronPlugin::hook_cron_post_schedule($this);
    module_invoke_all('cron_post_schedule', $this);
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
    return $this->getPlugin('launcher')->lock($this);
  }

  /**
   * Unlock job.
   */
  public function unlock($lock_id = NULL, $manual = FALSE) {
    if (!$lock_id) {
      $lock_id = $this->isLocked();
    }
    if ($lock_id) {
      return $this->getPlugin('launcher')->unlock($lock_id, $manual);
    }
  }

  /**
   * Get locked state of job.
   */
  public function isLocked() {
    return $this->getPlugin('launcher')->isLocked($this);
  }

  /**
   * Get locked state for multiple jobs.
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
    module_invoke_all('cron_post_launch', $this);
    $this->clearSignals();
    $this->setProgress(FALSE);
    return $result;
  }

  /**
   * Get log entries.
   */
  public function getLogEntries($limit = 10) {
    return $this->getPlugin('logger')->getLogEntries($this->name, $limit);
  }

  /**
   * Load log entry.
   */
  public function loadLogEntry($lock_id) {
    return $this->getPlugin('logger')->load($this->name, $lock_id);
  }

  /**
   * Load latest log.
   */
  public function loadLatestLogEntry() {
    return $this->getPlugin('logger')->load($this->name);
  }

  /**
   * Load latest log entries.
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
   * Get multiple job progresses.
   */
  static public function getProgresses($jobs) {
    $names = array();
    foreach ($jobs as $job) {
      $names[] = 'uc-progress:' . $job->name;
    }
    $values = _ultimate_cron_variable_load_multiple($names, FALSE);
    $result = array();
    foreach ($jobs as $job) {
      $result[$job->name] = isset($values['uc-progress:' . $job->name]) ? $values['uc-progress:' . $job->name] : FALSE;
    }
    return $result;
  }

  /**
   * Get job progress.
   */
  public function getProgress() {
    $name = 'uc-progress:' . $this->name;
    return _ultimate_cron_variable_load($name);
  }

  /**
   * Set job progress.
   */
  public function setProgress($progress) {
    $name = 'uc-progress:' . $this->name;
    _ultimate_cron_variable_save($name, $progress);
  }

}
