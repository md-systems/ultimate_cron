<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Logger\LoggerBase.
 */
namespace Drupal\ultimate_cron\Logger;
use Drupal\Core\Logger\RfcLogLevel;
use Drupal\Core\Url;
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
   * {@inheritdoc}
   */
  public function factoryLogEntry($name) {
    return new $this->logEntryClass($name, $this);
  }

  /**
   * {@inheritdoc}
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
   * {@inheritdoc}
   */
  public function catchMessages(LogEntry $log_entry) {
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
   * {@inheritdoc}
   */
  public function unCatchMessages(LogEntry $log_entry) {
    $class = get_class($this);
    unset($class::$log_entries[$log_entry->lid]);
  }

  /**
   * Invoke loggers watchdog hooks.
   *
   * @param LogEntry $log_entry
   *   Watchdog log entry array.
   */
  final static public function hook_watchdog(LogEntry $log_entry) {
    if (static::$log_entries) {
      foreach (static::$log_entries as $log_entry_object) {
        $log_entry_object->watchdog($log_entry);
      }
    }
  }

  /**
   * Log to ultimate cron logs only.
   *
   * @param string $type
   *   Category of the message.
   * @param string $message
   *   The message to store in the log. Keep $message translatable.
   * @param array $variables
   *   The variables for $message string to replace.
   * @param RfcLogLevel $severity
   *   (optional) The severity of th event.
   * @param Url $link
   *   A link to associate with the message.
   */
  final static public function log($type, $message, array $variables = [], RfcLogLevel $severity = NULL, Url $link = NULL) {
    if (static::$log_entries) {
      foreach (static::$log_entries as $log_entry_object) {
        $log_entry_object->log($type, $message, $variables, $severity, $link);
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  static public function catchMessagesShutdownWrapper($class) {
    if ($class::$log_entries) {
      foreach ($class::$log_entries as $log_entry) {
        $log_entry->logger->catchMessagesShutdown($log_entry);
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function catchMessagesShutdown(LogEntry $log_entry) {
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
   * {@inheritdoc}
   */
  public function loadLatestLogEntries(array $jobs, array $log_types) {
    $logs = array();
    foreach ($jobs as $job) {
      $logs[$job->id()] = $job->loadLatestLogEntry($log_types);
    }
    return $logs;
  }

  /**
   * {@inheritdoc}
   */
  abstract public function load($name, $lock_id = NULL, array $log_types = [ULTIMATE_CRON_LOG_TYPE_NORMAL]);

  /**
   * {@inheritdoc}
   */
  abstract public function getLogEntries($name, array $log_types, $limit = 10);
}
