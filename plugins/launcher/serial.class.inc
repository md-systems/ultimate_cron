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
      'max_launch_threads' => 1,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
    if (!$job) {
      $elements['max_execution_time'] = array(
        '#title' => t("Maximum execution time"),
        '#type' => 'textfield',
        '#default_value' => $values['max_execution_time'],
        '#description' => t('Maximum execution time for a cron run in seconds.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
      $elements['max_launch_threads'] = array(
        '#title' => t("Maximum number of launcher threads"),
        '#type' => 'textfield',
        '#default_value' => $values['max_launch_threads'],
        '#description' => t('The maximum number of launch threads that can be running at any given time.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
        '#element_validate' => array('element_validate_number'),
      );
    }
    else {
      $elements['no_settings'] = array(
        '#markup' => '<p>' . t('This plugin has no settings.') . '</p>',
      );
    }
  }

  function settingsFormValidate($form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
    if (intval($values['max_launch_threads']) <= 0) {
      form_set_error("settings[$this->type][$this->name", t('%title must be greater than 0', array(
        '%title' => $elements['max_launch_threads']['#title']
      )));
    }
  }

  static public function launchJobs($jobs) {
    // Set proper max execution time.
    $plugin = ultimate_cron_plugin_load('launcher', 'serial');
    $settings = $plugin->getDefaultSettings();

    $max_execution_time = ini_get('max_execution_time');
    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }

    dpm("Testing threads: " . $settings['max_launch_threads']);
    foreach ($jobs as $job) {
      $job->launch();
    }
  }
}
