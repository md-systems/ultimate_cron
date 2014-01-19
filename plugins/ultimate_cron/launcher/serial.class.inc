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
    ) + parent::defaultSettings();
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    parent::settingsForm($form, $form_state, $job);

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
      unset($elements['no_settings']);
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
      $init_message = t('Launched manually');
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

  static public function findFreeThread($lock, $lock_timeout = NULL, $timeout = 3) {
    $plugin = ultimate_cron_plugin_load('launcher', 'serial');
    $settings = $plugin->getDefaultSettings();

    // Find a free thread, try for 3 seconds.
    $delay = $timeout * 1000000;
    $sleep = 25000;

    do {
      for ($thread = 1; $thread <= $settings['max_launch_threads']; $thread++) {
        if ($thread == self::$currentThread) {
          continue;
        }

        $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
        if (!UltimateCronLock::isLocked($lock_name)) {
          if ($lock) {
            if (UltimateCronLock::lock($lock_name, $lock_timeout)) {
              error_log("Locked: $thread");
              return TRUE;
            }
          }
          else {
            error_log("Available: $thread");
            return $thread;
          }
        }
        if ($delay > 0) {
          error_log("Sleep: $sleep ($delay)");
          usleep($sleep);
          // After each sleep, increase the value of $sleep until it reaches
          // 500ms, to reduce the potential for a lock stampede.
          $delay = $delay - $sleep;
          $sleep = min(500000, $sleep + 25000, $delay);
        }
      }
    } while ($delay > 0);
    return FALSE;
  }

  /**
   * Launch manager.
   */
  static public function launchJobs($jobs, $launch_poorman = FALSE) {
    error_log(__FUNCTION__);
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

    if (!empty($_GET['thread'])) {
      $thread = intval($_GET['thread']);
      if ($thread < 1 || $thread > $settings['max_launch_threads']) {
        watchdog('serial_launcher', "Invalid thread available for starting launch thread", array(), WATCHDOG_WARNING);
        return;
      }

      $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
      $timeout = $launch_poorman ? 0 : 3;
      $thread = UltimateCronLock::lock($lock_name, $lock_timeout, $timeout) ? $thread : FALSE;
    }
    else {
      $thread = self::findFreeThread(TRUE, $lock_timeout, 0);
    }

    if (!$thread) {
      watchdog('serial_launcher', "No free threads available for launching jobs", array(), WATCHDOG_WARNING);
      return;
    }

    watchdog('serial_launcher', "Cron thread %thread started", array('%thread' => $thread), WATCHDOG_INFO);

    self::$currentThread = $thread;

    // Pre-emptively launch new poormans cron
    if ($launch_poorman) {
      ultimate_cron_poorman_page_flush();

      $thread = self::findFreeThread(FALSE, $lock_timeout, 0);
      error_log(self::$currentThread . " - PRE POORMAN THREAD: $thread");
      if ($thread) {
        ultimate_cron_trigger_poorman();
      }
    }

    foreach ($jobs as $job) {
      if ($job->schedule()) {
        $job->launch();
      }
    }

    // "Post-emptively" launch new poormans cron
    if ($launch_poorman) {
      $thread = self::findFreeThread(FALSE, $lock_timeout, 0);
      error_log(self::$currentThread . " - POST POORMAN THREAD: $thread");
      if ($thread) {
        ultimate_cron_trigger_poorman();
      }
    }
  }

  /**
   * Launch manager.
   */
  static public function launchPoorman($class, $jobs) {
    error_log(__FUNCTION__);
    return $class::launchJobs($jobs, TRUE);

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

    $thread = self::findFreeThread(FALSE);

    if (!$thread) {
      watchdog('serial_launcher', "No free threads available for launching jobs", array(), WATCHDOG_WARNING);
      return;
    }

    watchdog('serial_launcher', "Poorman cron thread %thread started", array('%thread' => $thread), WATCHDOG_INFO);
  }
}
