<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Logger\DatabaseLogEntry.
 */

namespace Drupal\ultimate_cron\Logger;

class DatabaseLogEntry extends LogEntry {

  /**
   * Save log entry.
   */
  public function save() {
    if (!$this->lid) {
      return;
    }

    static $retry = 0;

    try {
      db_insert('ultimate_cron_log')
        ->fields(array(
          'lid' => $this->lid,
          'name' => $this->name,
          'log_type' => $this->log_type,
          'start_time' => $this->start_time,
          'end_time' => $this->end_time,
          'uid' => $this->uid,
          'init_message' => $this->init_message,
          'message' => $this->message,
          'severity' => $this->severity
        ))
        ->execute();
    } catch (\Exception $e) {
      // Row already exists. Let's update it, if we can.
      $updated = db_update('ultimate_cron_log')
        ->fields(array(
          'name' => $this->name,
          'log_type' => $this->log_type,
          'start_time' => $this->start_time,
          'end_time' => $this->end_time,
          'init_message' => $this->init_message,
          'message' => $this->message,
          'severity' => $this->severity
        ))
        ->condition('lid', $this->lid)
        ->condition('end_time', 0)
        ->execute();
      if (!$updated) {
        // Row was not updated, someone must have beaten us to it.
        // Let's create a new log entry.
        $lid = $this->lid . '-' . uniqid('', TRUE);
        $this->message = t('Lock #@original_lid was already closed and logged. Creating a new log entry #@lid', array(
            '@original_lid' => $this->lid,
            '@lid' => $lid,
          )) . "\n" . $this->message;
        $this->severity = $this->severity >= 0 && $this->severity < WATCHDOG_ERROR ? $this->severity : WATCHDOG_ERROR;
        $this->lid = $lid;
        $retry++;
        if ($retry > 3) {
          $retry = 0;
          \Drupal::logger('database_logger')->critical((string) $e);
          return;
        }

        $this->save();
        $retry--;
      }
    }
  }
}
