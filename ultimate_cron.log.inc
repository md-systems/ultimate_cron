<?php

class UltimateCronLog {
  public $name;
  public $finished = FALSE;
  public $start_time = 0;
  public $end_time = 0;
  public $scheduler_message = '';
  public $launcher_message = '';
  public $module = 'ultimate_cron_log';
  public $messages = '';
  public $severity = -1;
  static public $logs = array();

  function __construct($name) {
    $this->name = $name;
  }

  function initialize() {
    $this->lid = NULL;
    $this->start_time = microtime(TRUE);
    $this->end_time = 0;
    $this->scheduler_message = '';
    $this->launcher_message = '';
    $this->messages = '';
    $this->severity = -1;
    return $this;
  }

  function create() {
    $this->initialize();
    $this->save();
  }

  function save() {
    $keys = $this->lid ? array('lid') : array();
    drupal_write_record('ultimate_cron_log', $this, $keys);
  }

  function finish() {
    $this->unCatchMessages();
    if (!$this->finished) {
      $this->end_time = microtime(TRUE);
      $this->finished = TRUE;
      $this->save();
    }
  }

  static function findLatest($name) {
    $log = db_select('ultimate_cron_log', 'l')
             ->fields('l')
             ->condition('l.name', $name)
             ->orderBy('l.start_time', 'DESC')
             ->range(0, 1)
             ->execute()
             ->fetchObject('UltimateCronLog', array($name));
    return $log;
  }

  function watchdog(array $log_entry) {
    $this->messages .= t($log_entry['message'], $log_entry['variables']) . "\n";
    if ($this->severity < 0 || $this->severity > $log_entry['severity']) {
      $this->severity = $log_entry['severity'];
    }
  }

  function catchMessages() {
    self::$logs[$this->lid] = $this;
    drupal_register_shutdown_function(array(get_class($this), 'catchMessagesShutdown'), $this);
  }

  function unCatchMessages() {
    unset(self::$logs[$this->lid]);
  }

  static function hook_watchdog(array $log_entry) {
    foreach (self::$logs as $log) {
      $log->watchdog($log_entry);
    }
  }

  static function catchMessagesShutdown($log) {
    if ($log->finished) {
      return;
    }

    // Get error messages.
    $error = error_get_last();
    if ($error) {
      $message = $error['message'] . ' (line ' . $error['line'] . ' of ' . $error['file'] . ').' . "\n";
      $severity = WATCHDOG_INFO;
      if ($error['type'] && (E_NOTICE || E_USER_NOTICE || E_USER_WARNING)) {
        $severity = WATCHDOG_NOTICE;
      }
      if ($error['type'] && (E_WARNING || E_CORE_WARNING || E_USER_WARNING)) {
        $severity = WATCHDOG_WARNING;
      }
      if ($error['type'] && (E_ERROR || E_CORE_ERROR || E_USER_ERROR || E_RECOVERABLE_ERROR)) {
        $severity = WATCHDOG_ERROR;
      }

      global $user;
      global $base_root;
      $log_entry = array(
        'type'        => $log->module,
        'message'     => $message,
        'variables'   => array(),
        'severity'    => $severity,
        'link'        => NULL,
        'user'        => $user,
        'uid'         => isset($user->uid) ? $user->uid : 0,
        'request_uri' => $base_root . request_uri(),
        'referer'     => isset($_SERVER['HTTP_REFERER']) ? $_SERVER['HTTP_REFERER'] : '',
        'ip'          => ip_address(),
        // Request time isn't accurate for long processes, use time() instead.
        'timestamp'   => time(),
      );

      $log->watchdog($log_entry);
    }
    $log->finish();
  }
}
