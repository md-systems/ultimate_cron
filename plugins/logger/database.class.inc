<?php

class UltimateCronDatabaseLogger extends UltimateCronLogger {

  function finish() {
  }

  function save() {
    $keys = $this->lid ? array('lid') : array();
    drupal_write_record('ultimate_cron_log', $this, $keys);
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
      $this->lid = $log->lid;
      $this->start_time = $log->start_time;
      $this->end_time = $log->end_time;
      $this->message = $log->message;
      $this->severity = $log->severity;
    }
    else {
      $this->lid = NULL;
      $this->start_time = 0;
      $this->end_time = 0;
      $this->message = '';
      $this->severity = -1;
    }
    $this->module = $job->hook['module'];
    $this->name = $job->name;
    $this->finished = TRUE;      
    return $this;
  }
}
