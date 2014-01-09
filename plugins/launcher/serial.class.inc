<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronSerialLauncher extends UltimateCronLauncher {
  public function launch($job) {
    // Log start.
    $log = $job->startLog();

    // Run job.
    try {
      $job->run();
    }
    catch (Exception $e) {
      watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => $e->getMessage()), WATCHDOG_ERROR);
    }

    // Log finish.
    $log->finish();
  }
}
