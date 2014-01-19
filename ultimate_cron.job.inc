<?php
/**
 * @file
 * Job class for Ultimate Cron.
 */

class UltimateCronJob {
  static public $signals;

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
      $settings += array($plugin->name => array());
      $settings['name'] = $plugin->name;
      $settings[$plugin->name] += $plugin->getDefaultSettings($this);
    }
    else {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $name => $plugin) {
        if ($plugin->isValid($this)) {
          $settings[$name] += $plugin->getDefaultSettings($this);
        }
      }
    }
    return $settings;
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
    switch ($this->hook['api_version']) {
      case 1:
        $args = array($this->hook);
        $args = array_merge($args, $this->hook['callback_arguments']);
        if (is_callable($this->hook['callback'])) {
          return call_user_func_array($this->hook['callback'], array(
            $this->name, $this->hook
          ));
        }
        else {
          return module_invoke($this->hook['module'], 'cronapi', 'execute', $this->name, $this->hook);
        }

      case 2:
        $result = call_user_func_array($this->hook['callback'], array(
          $this, $this->hook['callback_arguments']
        ));
        return $result;

      default:
        throw new Exception(t('Could not invoke cron job @name. Wrong API version (@api_version)', array(
          '@name' => $this->name,
          '@api_version' => $this->hook['api_version'],
        )));
    }
  }

  /**
   * Check job schedule.
   */
  public function schedule() {
    return empty($this->disabled) && $this->getPlugin('scheduler')->schedule($this);
  }

  /**
   * Launch job.
   */
  public function launch() {
    $this->clearSignals();
    return $this->getPlugin('launcher')->launch($this);
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
   * Run job.
   */
  public function run() {
    return $this->getPlugin('launcher')->run($this);
  }

  /**
   * Get log entries.
   */
  public function getLogEntries($limit = 10) {
    return $this->getPlugin('logger')->getLogEntries($this, $limit);
  }

  /**
   * Load log.
   */
  public function loadLog($lock_id) {
    return $this->getPlugin('logger')->load($this, $lock_id);
  }

  /**
   * Load latest log.
   */
  public function loadLatestLog() {
    return $this->getPlugin('logger')->loadLatest($this);
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
    $log = $this->getPlugin('logger');
    $log->initialize($lock_id, $this, $init_message);
    $log->catchMessages();
    return $log;
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
}
