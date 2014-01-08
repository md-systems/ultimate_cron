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
    $log = $job->createLogEntry();
    ultimate_cron_catch_messages($log);

    drupal_register_shutdown_function(array(get_class($this), 'catchErrors'), $job, $log);
    // Run job.
    try {
      $job->run();
    }
    catch (Exception $e) {
      watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => $e->getMessage()), WATCHDOG_ERROR);
    }

    // Log finish.
    $log->finish();
    ultimate_cron_uncatch_messages($log);
  }

  function catchErrors($job, $log) {
    if (!$log->finished) {
      $log->finish();
      ultimate_cron_uncatch_messages($log);
    }
  }
}
