<?php

class UltimateCronJob {
  function getScheduler() {
    if (!empty($this->settings['scheduler'])) {
      return ultimate_cron_scheduler_load($this->settings['scheduler']);
    }
    else {
      return ultimate_cron_scheduler_load($this->hook['scheduler']);
    }
  }

  function getSchedulerSettings() {
    return $this->settings['scheduler_settings'] + $this->getScheduler()->getDefaultSettings($this);
  }

  function getScheduledLabel() {
    return $this->getScheduler()->getScheduledLabel($this->getSchedulerSettings());
  }

  function getLauncher() {
    if (!empty($this->settings['launcher'])) {
      return ultimate_cron_launcher_load($this->settings['launcher']);
    }
    else {
      return ultimate_cron_launcher_load($this->hook['launcher']);
    }
  }

  function getLauncherSettings() {
    return $this->settings['launcher_settings'] + $this->hook['launcher_settings'];
  }

  function getLatestLogEntry() {
    return UltimateCronLog::findLatest($this->name);
  }

  function getLastRan() {
    $log = UltimateCronLog::findLatest($this->name);
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
    $log = new UltimateCronLog($this->name);
    $log->module = $this->hook['module'];
    $log->create();
    $log->catchMessages();
    return $log;
  }

  function createLogEntry() {
    $log = new UltimateCronLog($this->name);
    $log->create();
    return $log;
  }

  function getModuleName() {
    return ultimate_cron_module_name($this->hook['module']);
  }
}
