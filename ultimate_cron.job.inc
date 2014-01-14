<?php

class UltimateCronJob {
  function getSettings() {
    ctools_include('plugins');
    $settings = array();
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $plugin_info) {
      $settings[$plugin_info['type']] = $this->getPluginSettings($plugin_type);
    }
    return $settings;
  }

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
        $settings += array($name => array());
        $settings[$name] += $plugin->getDefaultSettings($this);
      }
    }
    return $settings;
  }

  function schedule() {
    return $this->getPlugin('scheduler')->schedule($this);
  }

  function launch() {
    return $this->getPlugin('launcher')->launch($this);
  }

  function lock() {
    $settings = $this->getSettings();
    return $this->getPlugin('launcher')->lock(
      $this, $settings['settings']['general']['lock_timeout']
    );
  }

  function unlock() {
    return $this->getPlugin('launcher')->unlock($this);
  }

  function isLocked() {
    return $this->getPlugin('launcher')->isLocked($this);
  }

  function run() {
    if (!$this->lock()) {
      drupal_set_message("Could not acquire log for %name. Already running?", array(
        '%name' => $this->name
      ));
      return;
    }
    $log = NULL;
    try {
      $log = $this->startLog();
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

        default:
          watchdog('ultimate_cron', 'Could not invoke cron job @name. Wrong API version (@api_version)', array(
            '@name' => $this->name,
            '@api_version' => $this->hook['api_version'],
          ));
      }
      $log->finish();
      $this->unlock();
    }
    catch (Exception $e) {
      watchdog('ultimate_cron', 'Error running @name: @error', array('@error' => $e->getMessage()), WATCHDOG_ERROR);
      if ($log) {
        $log->finish();
      }
      $this->unlock();
    }
  }

  function getLogEntries() {
    return $this->getPlugin('logger')->getLogEntries($this);
  }

  function startLog() {
    $log = $this->getPlugin('logger');
    $log->initialize($this);
    $log->catchMessages();
    return $log;
  }

  function getModuleName() {
    $info = ultimate_cron_get_module_info($this->hook['module']);
    return $info && !empty($info['name']) ? $info['name'] : $this->hook['module'];
  }

  function getModuleDescription() {
    $info = ultimate_cron_get_module_info($this->hook['module']);
    return $info && !empty($info['description']) ? $info['description'] : '';
  }

}
