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
    // Log start
    $log = $job->createLogEntry();

    // Run job
    $job->run();

    // Log finish
    $log->end_time = microtime(TRUE);
    $log->severity = 0;
    dpm($log);
    $log->save();
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
    );
  }

  /**
   * Settings form Ultimate Cron launcher.
   */
  public function settingsForm(&$form, &$form_state) {
  }
}
