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
