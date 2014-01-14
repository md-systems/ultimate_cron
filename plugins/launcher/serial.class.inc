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
      'max_execution_time' => 3600,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
    $elements['max_execution_time'] = array(
      '#title' => t("Maximum execution time"),
      '#type' => 'textfield',
      '#default_value' => $values['max_execution_time'],
      '#description' => t('Maximum execution time for a cron run in seconds.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }

  /**
   * Launcher.
   */
  public function launch($job) {
    // Set proper max execution time.
    $settings = $job->getPluginSettings('launcher');
    $max_execution_time = ini_get('max_execution_time');
    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }

    // Log start.
    // $log = $job->startLog();

    // Run job.
    try {
      $job->run();
    }
    catch (Exception $e) {
      watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => $e->getMessage()), WATCHDOG_ERROR);
    }

    // Log finish.
    // $log->finish();
  }
}
