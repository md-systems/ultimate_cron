<?php
/**
 * @file
 * A database-mediated implementation of a locking mechanism.
 *
 * Supports cross request persistance as well as GAP-LOCK mitigation.
 */

class UltimateCronLock {
  private static $locks = NULL;

  /**
   * Shutdown handler for releasing locks.
   */
  static public function shutdown() {
    foreach (array_keys(self::$locks) as $lock_id) {
      self::unlock($lock_id);
    }
  }

  /**
   * Dont release lock on shutdown.
   */
  static public function persist($lock_id) {
    if (isset(self::$locks)) {
      unset(self::$locks[$lock_id]);
    }
  }

  /**
   * Acquire lock.
   */
  static public function lock($name, $timeout = 30.0) {
    // First, ensure cleanup.
    if (!isset(self::$locks)) {
      self::$locks = array();
      drupal_register_shutdown_function(array(self, 'shutdown'));
    }

    try {
      // First we ensure that previous locks are "removed"
      // if they are expired.
      self::expire($name);

      // Ensure that the timeout is at least 1 ms.
      $timeout = max($timeout, 0.001);
      $expire = microtime(TRUE) + $timeout;

      // Now we try to acquire the lock.
      $lock_id = db_insert('ultimate_cron_lock')
        ->fields(array(
          'name' => $name,
          'current' => 0,
          'expire' => $expire,
        ))
        ->execute();

      self::$locks[$lock_id] = TRUE;

      return $lock_id;
    }
    catch (PDOException $e) {
      return FALSE;
    }
  }

  /**
   * Release lock if applicable.
   */
  public function expire($name) {
    if ($lock_id = self::isLocked($name)) {
      $now = microtime(TRUE);
      db_update('ultimate_cron_lock')
        ->expression('current', 'lid')
        ->condition('lid', $lock_id)
        ->condition('expire', $now, '<=')
        ->execute();
    }
  }

  /**
   * Release lock.
   */
  public function unlock($lock_id) {
    $unlocked = db_update('ultimate_cron_lock')
      ->expression('current', 'lid')
      ->condition('lid', $lock_id)
      ->condition('current', 0)
      ->execute();
    self::persist($lock_id);
    return $unlocked;
  }

  /**
   * Check if lock is taken.
   */
  public function isLocked($name) {
    // dpm(stripped_debug_backtrace());
    $now = microtime(TRUE);
    $result = db_select('ultimate_cron_lock', 'l')
      ->fields('l', array('lid', 'expire'))
      ->condition('name', $name)
      ->condition('current', 0)
      // ->condition('expire', $now, '>')
      ->execute()
      ->fetchObject();
    return $result && $result->expire > $now ? $result->lid : FALSE;
  }

  /**
   * Cleanup expired logs.
   */
  static public function cleanup() {
    $count = 0;
    $now = microtime(TRUE);

    $now = microtime(TRUE);
    db_update('ultimate_cron_lock')
      ->expression('current', 'lid')
      ->condition('expire', $now, '<=')
      ->execute();

    do {
      $lids = db_select('ultimate_cron_lock', 'l')
        ->fields('l', array('lid'))
        ->where('l.current = l.lid')
        // ->condition('expire', $now, '<=')
        ->range(0, 100)
        ->execute()
        ->fetchAll(PDO::FETCH_COLUMN);
      if ($lids) {
        $count += count($lids);
        db_delete('ultimate_cron_lock')
          ->condition('lid', $lids, 'IN')
          ->execute();
      }
    } while ($lids);
    watchdog('ultimate_cron_lock', 'Cleaned up @count expired logs', array(
      '@count' => $count
    ), WATCHDOG_INFO);
  }
}
