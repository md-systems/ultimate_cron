<?php

class UltimateCronDatabaseLogger extends UltimateCronLogger {
  function defaultSettings() {
    return array(
      'logs_expire' => 86400 * 14,
    );
  }

  function cleanup($job) {
    $settings = $job->getSettings();
    $logger_settings = $settings['logger'][$settings['logger']['name']];

    // Is cleanup disabled?
    if (!$logger_settings['logs_expire']) {
      return;
    }

    // Chunked delete.
    $count = 0;
    do {
      $lids = db_select('ultimate_cron_log', 'l')
        ->fields('l', array('lid'))
        ->condition('l.name', $job->name)
        ->condition('l.start_time', microtime(TRUE) - $logger_settings['logs_expire'], '<')
        ->range(0, 100)
        ->execute()
        ->fetchAll(PDO::FETCH_COLUMN);
      if ($lids) {
        $count += count($lids);
        db_delete('ultimate_cron_log')
          ->condition('lid', $lids, 'IN')
          ->execute();
      }
    } while ($lids);
    if ($count) {
      watchdog('database_logger', '@count log entries removed for job @name', array(
        '@count' => $count,
        '@name' => $job->name,
      ), WATCHDOG_INFO);
    }
  }

  function save() {
    $keys = $this->log_entry->lid ? array('lid') : array();
    drupal_write_record('ultimate_cron_log', $this->log_entry, $keys);
  }

  function loadLatest($job) {
    $log = db_select('ultimate_cron_log', 'l')
             ->fields('l')
             ->condition('l.name', $job->name)
             ->orderBy('l.start_time', 'DESC')
             ->range(0, 1)
             ->execute()
             ->fetchObject();
    if ($log) {
      $this->log_entry->lid = $log->lid;
      $this->log_entry->start_time = $log->start_time;
      $this->log_entry->end_time = $log->end_time;
      $this->log_entry->message = $log->message;
      $this->log_entry->severity = $log->severity;
    }
    else {
      $this->log_entry->lid = NULL;
      $this->log_entry->start_time = 0;
      $this->log_entry->end_time = 0;
      $this->log_entry->message = '';
      $this->log_entry->severity = -1;
    }
    $this->log_entry->module = $job->hook['module'];
    $this->log_entry->name = $job->name;
    $this->finished = TRUE;
    return $this;
  }

  function getLogEntries($job) {
    $logs = db_select('ultimate_cron_log', 'l')
      ->fields('l')
      ->extend('PagerDefault')
      ->condition('l.name', $job->name)
      ->limit(10)
      ->orderBy('l.start_time', 'DESC')
      ->execute()
      ->fetchAll();
    return $logs;
  }

  function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['logs_expire'] = array(
      '#type' => 'textfield',
      '#title' => t('Log entry expiration'),
      '#description' => t('Remove log entries older than X seconds. (0 = disabled)'),
      '#default_value' => $values['logs_expire'],
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }
}
