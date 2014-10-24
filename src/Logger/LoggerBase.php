<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Logger\LoggerBase.
 */
namespace Drupal\ultimate_cron\Logger;
use Drupal\Core\Logger\RfcLogLevel;
use Drupal\ultimate_cron\CronPlugin;
use Drupal\ultimate_cron\Logger\LogEntry;

/**
 * Abstract class for Ultimate Cron loggers
 *
 * Each logger must implement its own functions for getting/setting data
 * from the its storage backend.
 *
 * Abstract methods:
 *   load($name, $lock_id = NULL)
 *     - Load a log entry. If no $lock_id is provided, this method should
 *       load the latest log entry for $name.
 *
 * "Abstract" properties:
 *   $logEntryClass
 *     - The class name of the log entry class associated with this logger.
 */
abstract class LoggerBase extends CronPlugin implements LoggerInterface {
  static public $log_entries = NULL;
  public $logEntryClass = '\Drupal\ultimate_cron\Logger\LogEntry';

  /**
   * Factory method for creating a new unsaved log entry object.
   *
   * @param string $name
   *   Name of the log entry (name of the job).
   *
   * @return LogEntry
   *   The log entry.
   */
  public function factoryLogEntry($name) {
    return new $this->logEntryClass($name, $this);
  }

  /**
   * Create a new log entry.
   *
   * @param string $name
   *   Name of the log entry (name of the job).
   * @param string $lock_id
   *   The lock id.
   * @param string $init_message
   *   (optional) The initial message for the log entry.
   *
   * @return LogEntry
   *   The log entry created.
   */
  public function create($name, $lock_id, $init_message = '', $log_type = ULTIMATE_CRON_LOG_TYPE_NORMAL) {
    $log_entry = new $this->logEntryClass($name, $this, $log_type);

    $log_entry->lid = $lock_id;
    $log_entry->start_time = microtime(TRUE);
    $log_entry->init_message = $init_message;
    //$log_entry->save();
    return $log_entry;
  }


  /**
   * Begin capturing messages.
   *
   * @param LogEntry $log_entry
   *   The log entry that should capture messages.
   */
  public function catchMessages($log_entry) {
    $class = get_class($this);
    if (!isset($class::$log_entries)) {
      $class::$log_entries = array();
      // Since we may already be inside a drupal_register_shutdown_function()
      // we cannot use that. Use PHPs register_shutdown_function() instead.
      ultimate_cron_register_shutdown_function(array(
        $class,
        'catchMessagesShutdownWrapper'
      ), $class);
    }
    $class::$log_entries[$log_entry->lid] = $log_entry;
  }

  /**
   * End message capturing.
   *
   * Effectively disables the shutdown function for the given log entry.
   *
   * @param \Drupal\ultimate_cron\Logger\LogEntry $log_entry
   *   The log entry.
   */
  public function unCatchMessages($log_entry) {
    $class = get_class($this);
    unset($class::$log_entries[$log_entry->lid]);
  }

  /**
   * Invoke loggers watchdog hooks.
   *
   * @param array $log_entry
   *   Watchdog log entry array.
   */
  final static public function hook_watchdog(array $log_entry) {
    if (static::$log_entries) {
      foreach (static::$log_entries as $log_entry_object) {
        $log_entry_object->watchdog($log_entry);
      }
    }
  }

  /**
   * Log to ultimate cron logs only.
   *
   * @see watchdog()
   */
  final static public function log($type, $message, $variables = array(), $severity = RfcLogLevel::NOTICE, $link = NULL) {
    if (static::$log_entries) {
      foreach (static::$log_entries as $log_entry_object) {
        $log_entry_object->log($type, $message, $variables, $severity, $link);
      }
    }
  }

  /**
   * Shutdown handler wrapper for catching messages.
   *
   * @param string $class
   *   The class in question.
   */
  static public function catchMessagesShutdownWrapper($class) {
    if ($class::$log_entries) {
      foreach ($class::$log_entries as $log_entry) {
        $log_entry->logger->catchMessagesShutdown($log_entry);
      }
    }
  }

  /**
   * PHP shutdown function callback.
   *
   * Ensures that a log entry has been closed properly on shutdown.
   *
   * @param LogEntry $log_entry
   *   The log entry to close.
   */
  public function catchMessagesShutdown($log_entry) {
    $this->unCatchMessages($log_entry);

    if ($log_entry->finished) {
      return;
    }

    // Get error messages.
    $error = error_get_last();
    if ($error) {
      $message = $error['message'] . ' (line ' . $error['line'] . ' of ' . $error['file'] . ').' . "\n";
      $severity = RfcLogLevel::INFO;
      if ($error['type'] && (E_NOTICE || E_USER_NOTICE || E_USER_WARNING)) {
        $severity = RfcLogLevel::NOTICE;
      }
      if ($error['type'] && (E_WARNING || E_CORE_WARNING || E_USER_WARNING)) {
        $severity = RfcLogLevel::WARNING;
      }
      if ($error['type'] && (E_ERROR || E_CORE_ERROR || E_USER_ERROR || E_RECOVERABLE_ERROR)) {
        $severity = RfcLogLevel::ERROR;
      }

      $log_entry->log($log_entry->name, $message, array(), $severity);
    }
    $log_entry->finish();
  }

  /**
   * Load latest log entry for multiple jobs.
   *
   * This is the fallback method. Loggers should implement an optimized
   * version if possible.
   */
  public function loadLatestLogEntries($jobs, $log_types) {
    $logs = array();
    foreach ($jobs as $job) {
      $logs[$job->id()] = $job->loadLatestLogEntry($log_types);
    }
    return $logs;
  }

  /**
   * Load a log.
   *
   * @param string $name
   *   Name of log.
   * @param string $lock_id
   *   Specific lock id.
   *
   * @return \Drupal\ultimate_cron\Logger\LogEntry
   *   Log entry
   */
  abstract public function load($name, $lock_id = NULL, $log_types = array(ULTIMATE_CRON_LOG_TYPE_NORMAL));

  /**
   * Get page with log entries for a job.
   *
   * @param string $name
   *   Name of job.
   * @param array $log_types
   *   Log types to get.
   * @param integer $limit
   *   (optional) Number of log entries per page.
   *
   * @return array
   *   Log entries.
   */
  abstract public function getLogEntries($name, $log_types, $limit = 10);
}
