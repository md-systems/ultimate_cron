<?php
/**
 * @file
 * Functions for legacy handling of Background Process 1.x
 */

/**
 * Implements hook_background_process_shutdown().
 */
function ultimate_cron_background_process_shutdown($process = NULL, $msg = NULL) {
  // Some sanity checking.
  if (!$process || !is_object($process) || empty($process->callback)) {
    return;
  }
  // We only handle the legacy callback here.
  if ($process->callback != 'ultimate_cron_background_process_legacy_callback') {
    return;
  }

  // Close the Ultimate Cron log entry.
  list ($name, $lock_id) = $process->args;

  // If someone has set the dont log signal, it means that they
  // are handling the shutdown of the log.
  $job = ultimate_cron_job_load($name);
  if ($job->getSignal('background_process_legacy_dont_log')) {
    // Resend within this request, just in case.
    $job->sendSignal('background_process_legacy_dont_log');
    return;
  }

  $log_entry = $job->resumeLog($lock_id);

  // Rewrite message to conform with Ultimate Cron style, if this is
  // a manual unlock.
  global $user;
  if ($msg == t('Manually unlocked by !name', array('!name' => $user->name))) {
    $username = $user->uid ? $user->name : t('anonymous');
    $msg = t('@name manually unlocked by user @username (@uid)', array(
      '@name' => $name,
      '@username' => $username,
      '@uid' => $user->uid,
    ));
  }

  watchdog('ultimate_cron', $msg, array(), WATCHDOG_WARNING);
  $log_entry->finish();
}

/**
 * Background Process legacy callback for running cron jobs.
 */
function ultimate_cron_background_process_legacy_callback($name, $lock_id) {
  $job = ultimate_cron_job_load($name);

  $log_entry = $job->resumeLog($lock_id);

  // Run job.
  try {
    $settings = $job->getSettings('launcher');
    if ($settings['daemonize']) {
      $keepalive = TRUE;
      $expire = microtime(TRUE) + (float) $settings['daemonize_interval'];
      do {
        if ($job->getSignal('end_daemonize')) {
          watchdog('ultimate_cron', 'end daemonize signal recieved', array(), WATCHDOG_WARNING);
          $keepalive = FALSE;
          break;
        }

        $job->run();
        if ($settings['daemonize_delay']) {
          usleep(((float) $settings['daemonize_delay']) * 1000000);
        }
      } while (microtime(TRUE) < $expire);

      // Refresh disabled value.
      $job = ultimate_cron_job_load($name);

      if ($keepalive && empty($job->disabled)) {
        background_process_keepalive();

        // Save a copy of the log.
        $log_entry->lid = $lock_id . '-' . urlencode(uniqid('', TRUE));
        $job->sendSignal('background_process_legacy_dont_log');
        $log_entry->finish();

        // Restart log for keepalive.
        $log_entry->lid = $lock_id;
        $handle = background_process_current_handle();
        $process = background_process_get_process($handle);
        $log_entry->init_message = t('Re-launched at service host @name', array(
          '@name' => $process->service_host,
        ));

        $log_entry->message = '';
        $log_entry->end_time = 0;
        $log_entry->start_time = microtime(TRUE);
        $log_entry->save();
      }
      else {
        $job->sendSignal('background_process_legacy_dont_log');
        $log_entry->finish();
        $job->unlock($lock_id);
      }
    }
    else {
      $job->run();
      $job->sendSignal('background_process_legacy_dont_log');
      $log_entry->finish();
      $job->unlock($lock_id);
    }

  }
  catch (Exception $e) {
    watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => (string) $e), WATCHDOG_ERROR);
    $job->sendSignal('background_process_legacy_dont_log');
    $log_entry->finish();
    $job->unlock($lock_id);
  }
}
