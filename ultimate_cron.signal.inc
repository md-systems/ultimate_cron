<?php

/**
 * Get a signal without claiming it.
 */
function ultimate_cron_peek_signal($name, $signal) {
  return db_select('ultimate_cron_signal', 's')
    ->fields('s', array('job_name'))
    ->condition('job_name', $name)
    ->condition('signal_name', $signal)
    ->execute()
    ->fetchField();
}

/**
 * Get and claim signal.
 */
function ultimate_cron_get_signal($name, $signal) {
  $claimed = db_update('ultimate_cron_signal')
    ->fields(array('claimed' => 1))
    ->condition('job_name', $name)
    ->condition('signal_name', $signal)
    ->condition('claimed', 0)
    ->execute();
  if ($claimed) {
    db_delete('ultimate_cron_signal')
      ->condition('job_name', $name)
      ->condition('signal_name', $signal)
      ->condition('claimed', 1)
      ->execute();
    return TRUE;
  }
  return FALSE;
}

/**
 * Set signal.
 */
function ultimate_cron_set_signal($name, $signal) {
  return db_merge('ultimate_cron_signal')
    ->key(array(
      'job_name' => $name,
      'signal_name' => $signal,
    ))
    ->fields(array('claimed' => 0))
    ->execute();
}

/**
 * Clear signal.
 */
function ultimate_cron_clear_signal($name, $signal) {
  db_delete('ultimate_cron_signal')
    ->condition('job_name', $name)
    ->condition('signal_name', $signal)
    ->execute();
}

/**
 * Clear signals.
 */
function ultimate_cron_clear_signals($name) {
  db_delete('ultimate_cron_signal')
    ->condition('job_name', $name)
    ->execute();
}
