<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Progress.
 */

namespace Drupal\ultimate_cron;

class Progress {
  public $name;
  public $progressUpdated = 0;
  public $interval = 1;
  static public $instances = array();

  /**
   * Constructor.
   *
   * @param string $name
   *   Name of job.
   * @param float $interval
   *   How often the database should be updated with the progress.
   */
  public function __construct($name, $interval = 1) {
    $this->name = $name;
    $this->interval = $interval;
  }

  /**
   * Singleton factory.
   *
   * @param string $name
   *   Name of job.
   * @param float $interval
   *   How often the database should be updated with the progress.
   *
   * @return Progress
   *   The object.
   */
  static public function factory($name, $interval = 1) {
    if (!isset(self::$instances[$name])) {
      self::$instances[$name] = new Progress($name, $interval);
    }
    self::$instances[$name]->interval = $interval;
    return self::$instances[$name];
  }

  /**
   * Get job progress.
   *
   * @return float
   *   The progress of this job.
   */
  public function getProgress() {
    $name = 'uc-progress:' . $this->name;
    $value = \Drupal::keyValue('uc-progress')->get($name);
    return $value;
  }

  /**
   * Get multiple job progresses.
   *
   * @param array $names
   *   Job names to get progress for.
   *
   * @return array
   *   Progress of jobs, keyed by job name.
   */
  static public function getProgressMultiple($names) {
    $values = \Drupal::keyValue('uc-progress')->getMultiple($names);

    return $values;
  }

  /**
   * Set job progress.
   *
   * @param float $progress
   *   The progress (0 - 1).
   */
  public function setProgress($progress) {
    if (microtime(TRUE) >= $this->progressUpdated + $this->interval) {
      $name = 'uc-progress:' . $this->name;

      \Drupal::keyValue('uc-progress')->set($name, $progress);

      $this->progressUpdated = microtime(TRUE);
      return TRUE;
    }
    return FALSE;
  }
}
