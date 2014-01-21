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
      'max_threads' => 1,
      'thread' => 'any',
    ) + parent::defaultSettings();
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    parent::settingsForm($form, $form_state, $job);

    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    unset($elements['no_settings']);

    if (!$job) {
      $max_threads = $values['max_threads'];
      $elements['max_execution_time'] = array(
        '#title' => t("Maximum execution time"),
        '#type' => 'textfield',
        '#default_value' => $values['max_execution_time'],
        '#description' => t('Maximum execution time for a cron run in seconds.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
      $elements['max_threads'] = array(
        '#title' => t("Maximum number of launcher threads"),
        '#type' => 'textfield',
        '#default_value' => $max_threads,
        '#description' => t('The maximum number of launch threads that can be running at any given time.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
        '#element_validate' => array('element_validate_number'),
      );
    }
    else {
      $settings = $this->getDefaultSettings();
      $max_threads = $settings['max_threads'];
    }

    $options = array('any' => t('-- Any-- '));
    for ($i = 1; $i <= $max_threads; $i++) {
      $options[$i] = $i;
    }
    $elements['thread'] = array(
      '#title' => t("Run in thread"),
      '#type' => 'select',
      '#default_value' => $values['thread'],
      '#options' => $options,
      '#description' => t('Which thread to run when invoking with ?thread=N'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }

  /**
   * Settings form validator.
   */
  public function settingsFormValidate($form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!$job) {
      if (intval($values['max_threads']) <= 0) {
        form_set_error("settings[$this->type][$this->name", t('%title must be greater than 0', array(
          '%title' => $elements['max_threads']['#title']
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

    if ($this->currentThread) {
      $init_message = t('Launched in thread @current_thread', array(
        '@current_thread' => $this->currentThread,
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

    $log->finish();
    $job->unlock($lock_id);
  }

  /**
   * Find a free thread for running cron jobs.
   */
  public function findFreeThread($lock, $lock_timeout = NULL, $timeout = 3) {
    $settings = $this->getDefaultSettings();

    // Find a free thread, try for 3 seconds.
    $delay = $timeout * 1000000;
    $sleep = 25000;

    do {
      for ($thread = 1; $thread <= $settings['max_threads']; $thread++) {
        if ($thread == $this->currentThread) {
          continue;
        }

        $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
        if (!UltimateCronLock::isLocked($lock_name)) {
          if ($lock) {
            if ($lock_id = UltimateCronLock::lock($lock_name, $lock_timeout)) {
              return array($thread, $lock_id);
            }
          }
          else {
            return array($thread, FALSE);
          }
        }
        if ($delay > 0) {
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
    $settings = $this->getDefaultSettings();

    // Set proper max execution time.
    $max_execution_time = ini_get('max_execution_time');
    $lock_timeout = max($max_execution_time, $settings['max_execution_time']);

    // If infinite max execution, then we use a day for the lock.
    $lock_timeout = $lock_timeout ? $lock_timeout : 86400;

    if (!empty($_GET['thread'])) {
      $thread = intval($_GET['thread']);
      if ($thread < 1 || $thread > $settings['max_threads']) {
        watchdog('serial_launcher', "Invalid thread available for starting launch thread", array(), WATCHDOG_WARNING);
        return;
      }

      $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
      $timeout = 3;
      list($thread, $lock_id) = $this->findFreeThread(TRUE, $lock_timeout, $timeout);
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

    foreach ($jobs as $job) {
      $settings = $job->getSettings('launcher');
      $proper_thread = ($settings['thread'] == 'any') || ($settings['thread'] == $thread);
      if ($job->schedule() && $proper_thread) {
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
}
