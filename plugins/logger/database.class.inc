<?php

class UltimateCronDatabaseLogger extends UltimateCronLogger {
  function cleanup($job) {

  }

  function finish() {
    if (!$this->log_entry->finished) {
      $this->log_entry->end_time = microtime(TRUE);
      $this->log_entry->finished = TRUE;
      $this->save();
    }
  }

  function save() {
    $keys = $this->log_entry->lid ? array('lid') : array();
    error_log("Saving: $this->log_entry->name - $this->log_entry->lid");
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
    $this->module = $job->hook['module'];
    $this->name = $job->name;
    $this->finished = TRUE;
    return $this;
  }
}
