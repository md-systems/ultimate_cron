<?php

/**
 * Contains \Drupal\ultimate_cron\Logger\LoggerInterface.
 */

namespace Drupal\ultimate_cron\Logger;

use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Component\Plugin\PluginInspectionInterface;
use Drupal\Core\Logger\RfcLogLevel;
use Drupal\Core\Url;
use Drupal\Core\Plugin\PluginFormInterface;

/**
 * Defines a logger method.
 */
interface LoggerInterface extends PluginInspectionInterface, ConfigurablePluginInterface, PluginFormInterface {

  /**
   * Factory method for creating a new unsaved log entry object.
   *
   * @param string $name
   *   Name of the log entry (name of the job).
   *
   * @return LogEntry
   *   The log entry.
   */
  public function factoryLogEntry($name);

  /**
   * Create a new log entry.
   *
   * @param string $name
   *   Name of the log entry (name of the job).
   * @param string $lock_id
   *   The lock id.
   * @param string $init_message
   *   (optional) The initial message for the log entry.
   * @param int $log_type
   *   (optional) The log_type for the log entry.
   *
   * @return LogEntry
   *   The log entry created.
   */
  public function create($name, $lock_id, $init_message = '', $log_type = ULTIMATE_CRON_LOG_TYPE_NORMAL);


  /**
   * Begin capturing messages.
   *
   * @param LogEntry $log_entry
   *   The log entry that should capture messages.
   */
  public function catchMessages(LogEntry $log_entry);

  /**
   * End message capturing.
   *
   * Effectively disables the shutdown function for the given log entry.
   *
   * @param \Drupal\ultimate_cron\Logger\LogEntry $log_entry
   *   The log entry.
   */
  public function unCatchMessages(LogEntry $log_entry);

  /**
   * Invoke loggers watchdog hooks.
   *
   * @param LogEntry $log_entry
   *   Watchdog log entry array.
   */
  static public function hook_watchdog(LogEntry $log_entry);

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
  static public function log($type, $message, array $variables = [], RfcLogLevel $severity = RfcLogLevel::NOTICE, Url $link = NULL);

  /**
   * Shutdown handler wrapper for catching messages.
   *
   * @param string $class
   *   The class in question.
   */
  static public function catchMessagesShutdownWrapper($class);

  /**
   * PHP shutdown function callback.
   *
   * Ensures that a log entry has been closed properly on shutdown.
   *
   * @param LogEntry $log_entry
   *   The log entry to close.
   */
  public function catchMessagesShutdown(LogEntry $log_entry);

  /**
   * Load latest log entry for multiple jobs.
   *
   * This is the fallback method. Loggers should implement an optimized
   * version if possible.
   *
   * @param array $jobs
   *   Jobs for which the log entries should be loaded.
   * @param array $log_types
   *   Type of log messages to load.
   */
  public function loadLatestLogEntries(array $jobs, array $log_types);

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
  public function load($name, $lock_id = NULL, array $log_types = [ULTIMATE_CRON_LOG_TYPE_NORMAL]);

  /**
   * Get page with log entries for a job.
   *
   * @param string $name
   *   Name of job.
   * @param array $log_types
   *   Log types to get.
   * @param int $limit
   *   (optional) Number of log entries per page.
   *
   * @return array
   *   Log entries.
   */
  public function getLogEntries($name, array $log_types, $limit = 10);

}
