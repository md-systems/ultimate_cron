<?php
/**
 * @file
 * Serial cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronSerialLauncher extends UltimateCronLauncher {
  static public $currentThread = NULL;

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'max_execution_time' => 3600,
      'max_launch_threads' => 1,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!$job) {
      $elements['max_execution_time'] = array(
        '#title' => t("Maximum execution time"),
        '#type' => 'textfield',
        '#default_value' => $values['max_execution_time'],
        '#description' => t('Maximum execution time for a cron run in seconds.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
      $elements['max_launch_threads'] = array(
        '#title' => t("Maximum number of launcher threads"),
        '#type' => 'textfield',
        '#default_value' => $values['max_launch_threads'],
        '#description' => t('The maximum number of launch threads that can be running at any given time.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
        '#element_validate' => array('element_validate_number'),
      );
    }
    else {
      $elements['no_settings'] = array(
        '#markup' => '<p>' . t('This plugin has no settings.') . '</p>',
      );
    }
  }

  /**
   * Settings form validator.
   */
  public function settingsFormValidate($form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!$job) {
      if (intval($values['max_launch_threads']) <= 0) {
        form_set_error("settings[$this->type][$this->name", t('%title must be greater than 0', array(
          '%title' => $elements['max_launch_threads']['#title']
        )));
      }
    }
  }

  /**
   * Launcher.
   */
  public function launch($job) {
    $lock_id = $job->lock();

    if (!$lock_id) {
      return;
    }

    if (self::$currentThread) {
      $init_message = t('Launched in thread @current_thread', array(
        '@current_thread' => self::$currentThread,
      ));
    }
    else {
      global $user;
      $username = $user->uid ? $user->name : t('Anonymous');
      $init_message = t('Launched by @username', array(
        '@username' => $username,
      ));
    }
    $log = $job->startLog($lock_id, $init_message);

    drupal_set_message(t('@name: @init_message', array(
      '@name' => $job->name,
      '@init_message' => $init_message,
    )));

    // Run job.
    try {
      $job->run();
    }
    catch (Exception $e) {
      watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => $e->getMessage()), WATCHDOG_ERROR);
    }

    $job->unlock($lock_id);
    $log->finish();
  }

  /**
   * Launch manager.
   */
  static public function launchJobs($jobs) {
    $plugin = ultimate_cron_plugin_load('launcher', 'serial');
    $settings = $plugin->getDefaultSettings();

    // Set proper max execution time.
    $max_execution_time = ini_get('max_execution_time');
    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }
    $lock_timeout = max($max_execution_time, $settings['max_execution_time']);

    // If infinite max execution, then we use a day for the lock.
    $lock_timeout = $lock_timeout ? $lock_timeout : 86400;

    // Find a free slot, try for 3 seconds.
    $lock = FALSE;
    $delay = 3 * 1000000;
    $sleep = 25000;

    while ($delay > 0) {
      for ($slot = 1; $slot <= $settings['max_launch_threads']; $slot++) {
        $lock_name = 'ultimate_cron_serial_launcher_' . $slot;
        if (lock_may_be_available($lock_name)) {
          if ($lock = lock_acquire($lock_name, $lock_timeout)) {
            break 2;
          }
        }
        usleep($sleep);
        // After each sleep, increase the value of $sleep until it reaches
        // 500ms, to reduce the potential for a lock stampede.
        $delay = $delay - $sleep;
        $sleep = min(500000, $sleep + 25000, $delay);
      }
    }

    if (!$lock) {
      watchdog('serial_launcher', "No free slots available for starting launch thread", array(), WATCHDOG_WARNING);
      return;
    }

    watchdog('serial_launcher', "Cron thread %slot started", array('%slot' => $slot), WATCHDOG_INFO);

    self::$currentThread = $slot;
    foreach ($jobs as $job) {
      $job->launch();
    }
  }
}
