<?php
/**
 * Created by PhpStorm.
 * User: berdir
 * Date: 4/4/14
 * Time: 3:03 PM
 */

namespace Drupal\ultimate_cron;

use Drupal\ultimate_cron\Entity\CronJob;
use Exception;

/**
 * Abstract class for Ultimate Cron launchers
 *
 * A launcher is responsible for locking and launching/running a job.
 *
 * Abstract methods:
 *   lock($job)
 *     - Lock a job. This method must return the lock_id on success
 *       or FALSE on failure.
 *
 *   unlock($lock_id, $manual = FALSE)
 *     - Release a specific lock id. If $manual is set, then the release
 *       was triggered manually by a user.
 *
 *   isLocked($job)
 *     - Check if a job is locked. This method must return the current
 *     - lock_id for the given job, or FALSE if it is not locked.
 *
 *   launch($job)
 *     - This method launches/runs the given job. This method must handle
 *       the locking of job before launching it. Returns TRUE on successful
 *       launch, FALSE if not.
 *
 * Important methods:
 *   isLockedMultiple($jobs)
 *     - Check locks for multiple jobs. Each launcher should implement an
 *       optimized version of this method if possible.
 *
 *   launchJobs($jobs)
 *     - Launches the jobs provided to it. A default implementation of this
 *       exists, but can be overridden. It is assumed that this function
 *       checks the jobs schedule before launching and that it also handles
 *       locking wrt concurrency for the launcher itself.
 *
 *   launchPoorman()
 *     - Launches all scheduled jobs via the proper launcher for each jobs.
 *       This method only needs to be implemented if the launcher wishes to
 *       provide a poormans cron launching mechanism. It is assumed that
 *       the poormans cron launcher handles locking wrt concurrency, etc.
 */
abstract class LauncherBase extends CronPlugin {

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array();
  }

  /**
   * Lock job.
   *
   * @param CronJob $job
   *   The job to lock.
   *
   * @return string
   *   Lock ID or FALSE.
   */
  abstract public function lock($job);

  /**
   * Unlock a lock.
   *
   * @param string $lock_id
   *   The lock id to unlock.
   * @param boolean $manual
   *   Whether this is a manual unlock or not.
   *
   * @return boolean
   *   TRUE on successful unlock.
   */
  abstract public function unlock($lock_id, $manual = FALSE);

  /**
   * Check if a job is locked.
   *
   * @param CronJob $job
   *   The job to check.
   *
   * @return string
   *   Lock ID of the locked job, FALSE if not locked.
   */
  abstract public function isLocked($job);

  /**
   * Launch job.
   *
   * @param CronJob $job
   *   The job to launch.
   *
   * @return boolean
   *   TRUE on successful launch.
   */
  abstract public function launch($job);

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
  public function isLockedMultiple($jobs) {
    $lock_ids = array();
    foreach ($jobs as $name => $job) {
      $lock_ids[$name] = $this->isLocked($job);
    }
  }

  /**
   * Run the job.
   *
   * @param CronJob $job
   *   The job to run.
   */
  public function run($job) {
    // Prevent session information from being saved while cron is running.
//    $original_session_saving = drupal_save_session();
//    drupal_save_session(FALSE);

    // Force the current user to anonymous to ensure consistent permissions on
    // cron runs.
//    $original_user = $GLOBALS['user'];
//    $GLOBALS['user'] = drupal_anonymous_user();

    $php_self = NULL;
    try {
      // Signal to whomever might be listening, that we're cron!
      // @investigate Is this safe? (He asked knowingly ...)
      $php_self = $_SERVER['PHP_SELF'] ? $_SERVER['PHP_SELF'] : '';
      $_SERVER['PHP_SELF'] = 'cron.php';

      $job->invoke();

      // Restore state.
      $_SERVER['PHP_SELF'] = $php_self;
    } catch (Exception $e) {
      // Restore state.
      if (isset($php_self)) {
        $_SERVER['PHP_SELF'] = $php_self;
      }

      watchdog('ultimate_cron', 'Error running @name: @error', array(
        '@name' => $job->id(),
        '@error' => $e->getMessage(),
      ), WATCHDOG_ERROR);
    }
    // Restore the user.
//    $GLOBALS['user'] = $original_user;
//    drupal_save_session($original_session_saving);
  }

  /**
   * Default implementation of jobs launcher.
   *
   * @param array $jobs
   *   Array of UltimateCronJob to launch.
   */
  public function launchJobs($jobs) {
    foreach ($jobs as $job) {
      if ($job->isScheduled()) {
        $job->launch();
      }
    }
  }

  /**
   * Format running state.
   */
  public function formatRunning($job) {
    $file = drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass.png';
    $status = theme('image', array('path' => $file));
    $title = t('running');
    return array($status, $title);
  }

  /**
   * Format unfinished state.
   */
  public function formatUnfinished($job) {
    $file = drupal_get_path('module', 'ultimate_cron') . '/icons/lock_open.png';
    $status = theme('image', array('path' => $file));
    $title = t('unfinished but not locked?');
    return array($status, $title);
  }

  /**
   * Default implementation of formatProgress().
   *
   * @param CronJob $job
   *   Job to format progress for.
   *
   * @return string
   *   Formatted progress.
   */
  public function formatProgress($job, $progress) {
    $progress = $progress ? sprintf("(%d%%)", round($progress * 100)) : '';
    return $progress;
  }

  /**
   * Default implementation of initializeProgress().
   *
   * @param CronJob $job
   *   Job to initialize progress for.
   */
  public function initializeProgress($job) {
    $class = _ultimate_cron_get_class('progress');
    return $class::factory($job->id())->setProgress(FALSE);
  }

  /**
   * Default implementation of finishProgress().
   *
   * @param CronJob $job
   *   Job to finish progress for.
   */
  public function finishProgress($job) {
    $class = _ultimate_cron_get_class('progress');
    return $class::factory($job->id())->setProgress(FALSE);
  }

  /**
   * Default implementation of getProgress().
   *
   * @param CronJob $job
   *   Job to get progress for.
   *
   * @return float
   *   Progress for the job.
   */
  public function getProgress($job) {
    $class = _ultimate_cron_get_class('progress');
    return $class::factory($job->id())->getProgress();
  }

  /**
   * Default implementation of getProgressMultiple().
   *
   * @param CronJob $jobs
   *   Jobs to get progresses for, keyed by job name.
   *
   * @return array
   *   Progresses, keyed by job name.
   */
  public function getProgressMultiple($jobs) {
    $class = _ultimate_cron_get_class('progress');
    return $class::getProgressMultiple(array_keys($jobs));
  }

  /**
   * Default implementation of setProgress().
   *
   * @param CronJob $job
   *   Job to set progress for.
   * @param float $progress
   *   Progress (0-1).
   */
  public function setProgress($job, $progress) {
    $class = _ultimate_cron_get_class('progress');
    return $class::factory($job->id())->setProgress($progress);
  }

}
