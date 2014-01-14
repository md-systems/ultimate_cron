<?php

class UltimateCronJob {
  function getPlugin($plugin_type) {
    if (!empty($this->settings[$plugin_type]['name'])) {
      return ultimate_cron_plugin_load($plugin_type, $this->settings[$plugin_type]['name']);
    }
    else {
      return ultimate_cron_plugin_load($plugin_type, $this->hook[$plugin_type]['name']);
    }
  }

  function getPluginSettings($plugin_type) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    $plugin_info = $plugin_types['ultimate_cron'][$plugin_type];
    $static = $plugin_info['defaults']['static'];
    $class = $static['class'];

    if (!$class::$multiple) {
      $plugin_settings = $this->getPlugin($plugin_type)->getDefaultSettings($this);
    }
    else {
      /*
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      $plugin_settings = array();
      foreach ($plugins as $name => $plugin) {
        $plugin_settings += $plugin->getDefaultSettings($this);
      }
      */
    }
    // dpm($plugin_settings);
    // dpm($this->settings[$plugin_type]);
    return $this->settings[$plugin_type][$name] + $plugin_settings;
  }

  function getScheduler() {
    return $this->getPlugin('scheduler');
  }

  function getSchedulerSettings() {
    return $this->getPluginSettings('scheduler');
  }

  function getScheduledLabel() {
    $scheduler = $this->getScheduler();
    $settings = $this->getSchedulerSettings();
    return $this->getScheduler()->getScheduledLabel($settings[$scheduler->name]);
  }

  function getLauncher() {
    return $this->getPlugin('launcher');
  }

  function getLauncherSettings() {
    return $this->getPluginSettings('launcher');
  }

  function getLatestLog() {
    return $this->getPlugin('logger')->loadLatest($this);
  }

  function getLastRan() {
    $log = $this->getLatestLog();
    return $log ? $log->start_time : 0;
    #$log = ultimate_cron_get_last_log_entry($this->name);
    #return $log ? $log['start_time'] : 0;
  }

  function schedule() {
    $scheduler = $this->getScheduler();
    return $scheduler->schedule($this);
  }

  function launch() {
    $launcher = $this->getLauncher();
    return $launcher->launch($this);
  }

  function run() {
    dpm($this);
    $lock_key = "uc:" . $this->name;
    if (!lock_acquire($lock_key)) {
      drupal_set_message("Could not acquire log for %name. Already running?", array(
        '%name' => $this->name
      ));
    }
    switch ($this->hook['api_version']) {
      case 1:
        // $args = array($hook);
        // $args = array_merge($args, $this->hook['callback_arguments']);
        break;

      case 2:
        call_user_func_array($this->hook['callback'], array(
          $this, $this->hook['callback_arguments']
        ));
        break;
    }
    lock_release($lock_key);
  }

  function startLog() {
    $log = $this->getPlugin('logger');
    $log->initialize($this);
    $log->catchMessages();
    return $log;
  }

  function getModuleName() {
    return ultimate_cron_module_name($this->hook['module']);
  }
}
