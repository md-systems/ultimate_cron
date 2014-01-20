<?php
/**
 * @file
 * Serial cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronSerialLauncher extends UltimateCronLauncher {
  public $currentThread = NULL;

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
    error_log("serial->launch");

    $lock_id = $job->lock();

    if (!$lock_id) {
      error_log("no lock for $job->name");
      return;
    }

    if ($this->currentThread) {
      $init_message = t('Launched in thread @current_thread', array(
        '@current_thread' => $this->currentThread,
      ));
    }
    else {
      $init_message = t('Launched manually');
    }
    $log = $job->startLog($lock_id, $init_message);
    error_log("is log finished?: " . $log->finished);

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
    error_log("FINISHING LOG!!!");
    // error_log(print_r($log, TRUE));
    error_log("is log finished again?: " . $log->finished);
    $log->finish();
  }

  public function findFreeThread($lock, $lock_timeout = NULL, $timeout = 3) {
    $settings = $this->getDefaultSettings();

    // Find a free thread, try for 3 seconds.
    $delay = $timeout * 1000000;
    $sleep = 25000;

    do {
      for ($thread = 1; $thread <= $settings['max_launch_threads']; $thread++) {
        if ($thread == $this->currentThread) {
          continue;
        }

        $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
        if (!UltimateCronLock::isLocked($lock_name)) {
          if ($lock) {
            if ($lock_id = UltimateCronLock::lock($lock_name, $lock_timeout)) {
              error_log("Locked: $thread");
              return array($thread, $lock_id);
            }
          }
          else {
            error_log("Available: $thread");
            return array($thread, FALSE);
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
    return array(FALSE, FALSE);
  }

  /**
   * Launch manager.
   */
  public function launchJobs($jobs) {
    error_log(__FUNCTION__);
    $settings = $this->getDefaultSettings();

    // Set proper max execution time.
    $max_execution_time = ini_get('max_execution_time');
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
      $timeout = 3;
      $thread = UltimateCronLock::lock($lock_name, $lock_timeout, $timeout) ? $thread : FALSE;
    }
    else {
      list($thread, $lock_id) = $this->findFreeThread(TRUE, $lock_timeout, 0);
      $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
    }
    $this->currentThread = $thread;

    if (!$thread) {
      watchdog('serial_launcher', "No free threads available for launching jobs", array(), WATCHDOG_WARNING);
      return;
    }

    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }

    watchdog('serial_launcher', "Cron thread %thread started", array('%thread' => $thread), WATCHDOG_INFO);

    _ultimate_cron_variable_save('cron_last', time());

    foreach ($jobs as $job) {
      if ($job->schedule()) {
        $job->launch();
        // Be friendly, and check if we still own the lock.
        // If we don't, bail out, since someone else is handling
        // this thread.
        if ($lock_id !== UltimateCronLock::isLocked($lock_name)) {
          return;
        }
      }
    }
  }

  /**
   * Launch manager.
   */
  public function launchPoorman($jobs) {
    $cron_last = variable_get('cron_last', 0);
    $time = time();

    $settings = $this->getDefaultSettings();
    error_log(print_r($settings, TRUE));

    // Set proper max execution time.
    $max_execution_time = ini_get('max_execution_time');
    $lock_timeout = max($max_execution_time, $settings['max_execution_time']);

    // If infinite max execution, then we use a day for the lock.
    $lock_timeout = $lock_timeout ? $lock_timeout : 86400;

    // Locate available thread.
    list($thread, $lock_id) = $this->findFreeThread(TRUE, $lock_timeout, 0);

    if (!$thread) {
      error_log("NO FREE THREAD FOR POORMANS CRON");
      return;
    }
    ultimate_cron_poorman_page_flush();

    $this->currentThread = $thread;

    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }

    $cron_last = variable_get('cron_last', 0);
    $cron_next = floor(($cron_last + 60) / 60) * 60;
    $time = time();
    error_log(date('Y-m-d H:i:s', $cron_last));
    error_log(date('Y-m-d H:i:s', $time));
    error_log(date('Y-m-d H:i:s', $cron_next));
    if ($time < $cron_next) {
      error_log("NOT DUE YET. MUST SLEEP FOR: " . ($cron_next - $time));
      sleep($cron_next - $time + ($tread - 1));
    }

    watchdog('serial_launcher', "Poorman Cron thread %thread started", array('%thread' => $thread), WATCHDOG_INFO);

    _ultimate_cron_variable_save('cron_last', time());

    error_log("RUNNING JOBS");
    foreach ($jobs as $job) {
      error_log("CHECKING: $job->name");
      if ($job->schedule()) {
        error_log("RUNNING: $job->name");
        $job->launch();
      }
    }
    error_log("DONE RUNNING JOBS");
/*    
    error_log("MUST RUN JOBS HERE");
    for ($i = 0; $i < 30; $i++) {
      error_log("THREAD $thread running jobs - $i");
      sleep(1);
    }
*/
    UltimateCronLock::unlock($lock_id);

    // Re-launch poormans cron.
    ultimate_cron_trigger_poorman();
  }
}
