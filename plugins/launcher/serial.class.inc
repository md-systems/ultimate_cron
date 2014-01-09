<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronSerialLauncher extends UltimateCronLauncher {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'max_execution_time' => 86400,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $form['settings']['launcher_settings']['max_execution_time'] = array(
      '#title' => t("Maximum execution time"),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings']['launcher_settings']['max_execution_time'],
      '#description' => t('Maximum execution time for a cron run in seconds.'),
      '#fallback' => TRUE,
      '#fallback_required' => TRUE,
    );
  }

  /**
   * Launcher.
   */
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
