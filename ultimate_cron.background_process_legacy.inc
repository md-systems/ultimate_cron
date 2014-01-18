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

  if (!empty($GLOBALS['background_process_legacy_dont_log'][$lock_id])) {
    return;
  }

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

  $job = ultimate_cron_job_load($name);

  $log = $job->loadLog($lock_id);
  $log->finished = FALSE;
  $log->catchMessages(FALSE);
  watchdog('ultimate_cron', $msg, array(), WATCHDOG_WARNING);
  $log->catchMessagesShutdown();
}

/**
 * Background Process legacy callback for running cron jobs.
 */
function ultimate_cron_background_process_legacy_callback($name, $lock_id) {
  $job = ultimate_cron_job_load($name);

  $log = $job->loadLog($lock_id);
  $log->finished = FALSE;
  $log->catchMessages();

  // Run job.
  try {
    $job->run();
  }
  catch (Exception $e) {
    watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => $e->getMessage()), WATCHDOG_ERROR);
  }

  $GLOBALS['background_process_legacy_dont_log'][$lock_id] = TRUE;
  $job->unlock($lock_id);
  $log->finish();
}
