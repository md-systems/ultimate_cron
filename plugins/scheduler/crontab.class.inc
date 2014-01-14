<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

/**
 * Crontab scheduler.
 */
class UltimateCronCrontabScheduler extends UltimateCronScheduler {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'rules' => array('*/10+@ * * * *'),
      'catch_up' => '300',
    );
  }

  /**
   * Label for settings.
   */
  public function settingsLabel($name, $value) {
    switch ($name) {
      case 'rules':
        return $value ? implode(', ', $value) : '';
    }
    return parent::settingsLabel($name, $value);
  }

  /**
   * Form value for settings.
   */
  public function settingsFormValue($name, $value) {
    switch ($name) {
      case 'rules':
        return $value ? implode(',', $value) : '';
    }
    return parent::settingsFormValue($name, $value);
  }

  public function getScheduledLabel($settings) {
    return implode(', ', $settings['rules']);
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
    $rules = $this->settingsFormValue('rules', $values['rules']);
    $elements['rules'] = array(
      '#title' => t("Rules"),
      '#type' => 'textfield',
      '#default_value' => $rules,
      '#description' => t('Comma separated list of crontab rules.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#element_validate' => array('ultimate_cron_plugin_crontab_element_validate_rule'),
    );
    $elements['catch_up'] = array(
      '#title' => t("Catch up"),
      '#type' => 'textfield',
      '#default_value' => $values['catch_up'],
      '#description' => t('Dont run job after X seconds of rule.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }

  /**
   * Submit handler.
   */
  public function settingsFormSubmit(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
    if (!empty($values['rules'])) {
      $rules = explode(',', $values['rules']);
      $values['rules'] = array_map('trim', $rules);      
    }
  }

  public function schedule($job) {
    #dpm($job);
    include_once drupal_get_path('module', 'ultimate_cron') . '/CronRule.class.php';
    $settings = $job->getSchedulerSettings();
    #dpm($job->getScheduler()->name);
    #dpm($settings);
    foreach ($settings['rules'] as $rule) {
      $now = time();
      $cron = new CronRule($rule, $now);
      $cron_last_ran = $cron->getLastRan();
      $job_last_ran = $job->getLastRan();
      $msg = sprintf("Checking %s on %s < %s >= %s <= %s",
        $rule,
        date('Y-m-d H:i:s', $job_last_ran),
        date('Y-m-d H:i:s', $cron_last_ran),
        date('Y-m-d H:i:s', $now),
        date('Y-m-d H:i:s', $cron_last_ran + $settings['catch_up'])
      );
      dpm($msg, $job->name);
      #dpm("CHECKING: $rule with result: $last_ran", $job->name);

      if (
        $cron_last_ran >= $job_last_ran &&
        $now >= $job_last_ran &&
        $now <= $cron_last_ran + $settings['catch_up']
      ) {
        return TRUE;
      }
      return FALSE;
    }
    return FALSE;
  }
}
