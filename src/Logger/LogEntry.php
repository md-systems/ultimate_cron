<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Logger\LogEntry.
 */

namespace Drupal\ultimate_cron\Logger;

use Drupal\Component\Utility\SafeMarkup;
use Drupal\Component\Utility\String;
use Drupal\Core\Logger\RfcLogLevel;

/**
 * Abstract class for Ultimate Cron log entries.
 *
 * Each logger must implement its own log entry class based on this one.
 *
 * Abstract methods:
 *   save()
 *     - Save the actual log entry to whereever you please.
 *
 * Important properties:
 *   $log_entry_size
 *     - The maximum number of characters of the message in the log entry.
 */
abstract class LogEntry {
  public $lid = NULL;
  public $name = '';
  public $log_type = ULTIMATE_CRON_LOG_TYPE_NORMAL;
  public $uid = NULL;
  public $start_time = 0;
  public $end_time = 0;
  public $init_message = '';
  public $message = '';
  public $severity = -1;

  // Default 1MiB log entry.
  public $log_entry_size = 1048576;

  public $log_entry_fields = array(
    'lid',
    'uid',
    'log_type',
    'start_time',
    'end_time',
    'init_message',
    'message',
    'severity',
  );

  public $logger;
  public $job;
  public $finished = FALSE;

  /**
   * Constructor.
   *
   * @param string $name
   *   Name of log.
   * @param \Drupal\ultimate_cron\Logger\LoggerBase $logger
   *   A logger object.
   */
  public function __construct($name, $logger, $log_type = ULTIMATE_CRON_LOG_TYPE_NORMAL) {
    $this->name = $name;
    $this->logger = $logger;
    $this->log_type = $log_type;
    if (!isset($this->uid)) {
      $this->uid = \Drupal::currentUser()->id();
    }
  }

  /**
   * Get current log entry data as an associative array.
   *
   * @return array
   *   Log entry data.
   */
  public function getData() {
    $result = array();
    foreach ($this->log_entry_fields as $field) {
      $result[$field] = $this->$field;
    }
    return $result;
  }

  /**
   * Set current log entry data from an associative array.
   *
   * @param array $data
   *   Log entry data.
   */
  public function setData($data) {
    foreach ($this->log_entry_fields as $field) {
      if (array_key_exists($field, $data)) {
        $this->$field = $data[$field];
      }
    }
  }

  /**
   * Finish a log and save it if applicable.
   */
  public function finish() {
    if (!$this->finished) {
      $this->logger->unCatchMessages($this);
      $this->end_time = microtime(TRUE);
      $this->finished = TRUE;
      $this->save();
    }
  }

  /**
   * Implements hook_watchdog().
   *
   * Capture watchdog message and append it to the log entry.
   */
  public function watchdog(array $log_entry) {
    if (isset($log_entry['variables']) && is_array($log_entry['variables'])) {
      $this->message .= t($log_entry['message'], $log_entry['variables']) . "\n";
    }
    else {
      $this->message .= $log_entry['message'];
    }
    if ($this->severity < 0 || $this->severity > $log_entry['severity']) {
      $this->severity = $log_entry['severity'];
    }
    // Make sure that message doesn't become too big.
    if (mb_strlen($this->message) > $this->log_entry_size) {
      while (mb_strlen($this->message) > $this->log_entry_size) {
        $firstline = mb_strpos(rtrim($this->message, "\n"), "\n");
        if ($firstline === FALSE || $firstline == mb_strlen($this->message)) {
          // Only one line? That's a big line ... truncate it without mercy!
          $this->message = mb_substr($this->message, -$this->log_entry_size);
          break;
        }
        $this->message = substr($this->message, $firstline + 1);
      }
      $this->message = '.....' . $this->message;
    }
  }

  /**
   * Re-implementation of watchdog().
   *
   * @see watchdog()
   */
  public function log($type, $message, $variables = array(), $severity = RfcLogLevel::NOTICE, $link = NULL) {
    global $user, $base_root;

    // The user object may not exist in all conditions, so 0 is substituted if needed.
    $user_uid = isset($user->uid) ? $user->uid : 0;

    // Prepare the fields to be logged.
    $log_entry = array(
      'type' => $type,
      'message' => $message,
      'variables' => $variables,
      'severity' => $severity,
      'link' => $link,
      'user' => $user,
      'uid' => $user_uid,
      'request_uri' => $base_root . \Drupal::requestStack()->getCurrentRequest()->getUri(),
      'referer' => isset($_SERVER['HTTP_REFERER']) ? $_SERVER['HTTP_REFERER'] : '',
      //'ip' => ip_address(), @todo: Check this
      // Request time isn't accurate for long processes, use time() instead.
      'timestamp' => time(),
    );
    $this->watchdog($log_entry);
  }

  /**
   * Start catching watchdog messages.
   */
  public function catchMessages() {
    return $this->logger->catchMessages($this);
  }

  /**
   * Stop catching watchdog messages.
   */
  public function unCatchMessages() {
    return $this->logger->unCatchMessages($this);
  }

  /**
   * Get duration.
   */
  public function getDuration() {
    $duration = 0;
    if ($this->start_time && $this->end_time) {
      $duration = (int) ($this->end_time - $this->start_time);
    }
    elseif ($this->start_time) {
      $duration = (int) (microtime(TRUE) - $this->start_time);
    }
    return $duration;
  }

  /**
   * Format duration.
   */
  public function formatDuration() {
    $duration = $this->getDuration();
    switch (TRUE) {
      case $duration >= 86400:
        $format = 'd H:i:s';
        break;

      case $duration >= 3600:
        $format = 'H:i:s';
        break;

      default:
        $format = 'i:s';
    }
    return isset($duration) ? gmdate($format, $duration) : t('N/A');
  }

  /**
   * Format start time.
   */
  public function formatStartTime() {
    return $this->start_time ? format_date((int) $this->start_time, 'custom', 'Y-m-d H:i:s') : t('Never');
  }

  /**
   * Format end time.
   */
  public function formatEndTime() {
    return $this->end_time ? t('Previous run finished @ @end_time', array(
      '@end_time' => format_date((int) $this->end_time, 'custom', 'Y-m-d H:i:s')
    )) : '';
  }

  /**
   * Format user.
   */
  public function formatUser() {
    $username = t('anonymous') . ' (0)';
    if ($this->uid) {
      $user = user_load($this->uid);
      $username = $user ? SafeMarkup::format('@username (@uid)', array('@username' => $user->getUsername(), '@uid' => $user->id())) : t('N/A');
    }
    return $username;
  }

  /**
   * Format initial message.
   */
  public function formatInitMessage() {
    if ($this->start_time) {
      return $this->init_message ? $this->init_message . ' ' . t('by') . ' ' . $this->formatUser() : t('N/A');
    }
    else {
      $registered = variable_get('ultimate_cron_hooks_registered', array());
      return !empty($registered[$this->name]) ? t('Registered at @datetime', array(
        '@datetime' => format_date($registered[$this->name], 'custom', 'Y-m-d H:i:s'),
      )) : t('N/A');
    }
  }

  /**
   * Format severity.
   */
  public function formatSeverity() {
    switch ($this->severity) {
      case RfcLogLevel::EMERGENCY:
      case RfcLogLevel::ALERT:
      case RfcLogLevel::CRITICAL:
      case RfcLogLevel::ERROR:
        $file = 'misc/message-16-error.png';
        break;

      case RfcLogLevel::WARNING:
        $file = 'misc/message-16-warning.png';
        break;

      case RfcLogLevel::NOTICE:
        $file = 'misc/message-16-info.png';
        break;

      case RfcLogLevel::INFO:
      case RfcLogLevel::DEBUG:
      default:
        $file = 'misc/message-16-ok.png';
    }
    $status = theme('image', array('path' => $file));
    $severity_levels = array(
        -1 => t('no info'),
      ) + watchdog_severity_levels();
    $title = $severity_levels[$this->severity];
    return array($status, $title);
  }

  /**
   * Save log entry.
   */
  abstract public function save();
}
