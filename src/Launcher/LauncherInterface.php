<?php

/**
 * Contains \Drupal\ultimate_cron\Launcher\LauncherInterface.
 */

namespace Drupal\ultimate_cron\Launcher;

use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Component\Plugin\PluginInspectionInterface;
use Drupal\Core\Plugin\PluginFormInterface;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * Defines a launcher method.
 */
interface LauncherInterface extends PluginInspectionInterface, ConfigurablePluginInterface, PluginFormInterface {

  /**
   * Default settings.
   */
  public function defaultConfiguration();

  /**
   * Lock job.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job to lock.
   *
   * @return string
   *   Lock ID or FALSE.
   */
  public function lock(CronJob $job);

  /**
   * Unlock a lock.
   *
   * @param string $lock_id
   *   The lock id to unlock.
   * @param bool $manual
   *   Whether this is a manual unlock or not.
   *
   * @return bool
   *   TRUE on successful unlock.
   */
  public function unlock($lock_id, $manual = FALSE);

  /**
   * Check if a job is locked.
   *
   * @param CronJob $job
   *   The job to check.
   *
   * @return string
   *   Lock ID of the locked job, FALSE if not locked.
   */
  public function isLocked(CronJob $job);

  /**
   * Launch job.
   *
   * @param CronJob $job
   *   The job to launch.
   *
   * @return bool
   *   TRUE on successful launch.
   */
  public function launch(CronJob $job);

  /**
   * Fallback implementation of multiple lock check.
   *
   * Each launcher should implement an optimized version of this method
   * if possible.
   *
   * @param array $jobs
   *   Array of UltimateCronJob to check.
   *
   * @return array
   *   Array of lock ids, keyed by job name.
   */
  public function isLockedMultiple(array $jobs);

  /**
   * Run the job.
   *
   * @param CronJob $job
   *   The job to run.
   */
  public function run(CronJob $job);

  /**
   * Default implementation of jobs launcher.
   *
   * @param array $jobs
   *   Array of UltimateCronJob to launch.
   */
  public function launchJobs(array $jobs);

  /**
   * Format running state.
   *
   * @param CronJob $job
   *   The running job to format.
   */
  public function formatRunning(CronJob $job);

  /**
   * Format unfinished state.
   */
  public function formatUnfinished(CronJob $job);

  /**
   * Default implementation of formatProgress().
   *
   * @param CronJob $job
   *   Job to format progress for.
   * @param string $progress
   *   Formatted progress.
   */
  public function formatProgress(CronJob $job, $progress);

  /**
   * Default implementation of initializeProgress().
   *
   * @param CronJob $job
   *   Job to initialize progress for.
   */
  public function initializeProgress(CronJob $job);

  /**
   * Default implementation of finishProgress().
   *
   * @param CronJob $job
   *   Job to finish progress for.
   */
  public function finishProgress(CronJob $job);

  /**
   * Default implementation of getProgress().
   *
   * @param CronJob $job
   *   Job to get progress for.
   *
   * @return float
   *   Progress for the job.
   */
  public function getProgress(CronJob $job);

  /**
   * Default implementation of getProgressMultiple().
   *
   * @param array $jobs
   *   Jobs to get progresses for, keyed by job name.
   *
   * @return array
   *   Progresses, keyed by job name.
   */
  public function getProgressMultiple(array $jobs);

  /**
   * Default implementation of setProgress().
   *
   * @param CronJob $job
   *   Job to set progress for.
   * @param float $progress
   *   Progress (0-1).
   */
  public function setProgress(CronJob $job, $progress);

}
