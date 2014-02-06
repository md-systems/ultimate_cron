<?php
/**
 * @file
 * Crontab cron job scheduler for Ultimate Cron.
 */

/**
 * Crontab scheduler.
 */
class UltimateCronCrontabScheduler extends UltimateCronScheduler {
  private $skews = array();

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
   * Label for schedule.
   */
  public function formatLabel($job) {
    $settings = $job->getSettings($this->type);
    return implode("\n", $settings['rules']);
  }

  /**
   * Label for schedule.
   */
  public function formatLabelVerbose($job) {
    $settings = $job->getSettings($this->type);
    $parsed = array();

    foreach ($settings['rules'] as $rule) {
      $cron = CronRule::factory($rule, time(), $this->getSkew($job));
      $parsed[] = $cron->parseRule();
    }
    return implode("\n", $parsed);
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $rules = is_array($values['rules']) ? implode(',', $values['rules']) : '';

    $elements['rules'] = array(
      '#title' => t("Rules"),
      '#type' => 'textfield',
      '#default_value' => $rules,
      '#description' => t('Comma separated list of crontab rules.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#element_validate' => array('ultimate_cron_plugin_crontab_element_validate_rule'),
    );
    $elements['rules_help'] = array(
      '#type' => 'fieldset',
      '#title' => t('Rules help'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $elements['rules_help']['info'] = array(
      '#markup' => file_get_contents(drupal_get_path('module', 'ultimate_cron') . '/help/rules.html'),
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
  public function settingsFormSubmit(&$form, &$form_state, $job = NULL) {
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!empty($values['rules'])) {
      $rules = explode(',', $values['rules']);
      $values['rules'] = array_map('trim', $rules);
    }
  }

  /**
   * Schedule handler.
   */
  public function isScheduled($job) {
    $settings = $job->getSettings($this->type);
    $log_entry = isset($job->log_entry) ? $job->log_entry : $job->loadLatestLogEntry();
    $skew = $this->getSkew($job);
    $class = get_class($this);
    return $class::shouldRun($settings['rules'], $log_entry->start_time, NULL, $settings['catch_up'], $skew) ? TRUE : FALSE;
  }

  /**
   * Check crontab rules against times.
   */
  static public function shouldRun($rules, $job_last_ran, $time = NULL, $catch_up = 0, $skew = 0) {
    $time = is_null($time) ? time() : $time;
    foreach ($rules as $rule) {
      $cron = CronRule::factory($rule, $time, $skew);
      $cron_last_ran = $cron->getLastSchedule();

      if ($job_last_ran < $cron_last_ran && $cron_last_ran <= $time) {
        if ($time <= $cron_last_ran + $catch_up) {
          return $time - $job_last_ran;
        }
      }
    }
    return FALSE;
  }

  /**
   * Determine if job is behind schedule.
   */
  public function isBehind($job) {
    // Disabled jobs are not behind!
    if (!empty($job->disabled)) {
      return FALSE;
    }

    $log_entry = isset($job->log_entry) ? $job->log_entry : $job->loadLatestLogEntry();
    // If job hasn't run yet, then who are we to say it's behind its schedule?
    if (!$log_entry->start_time) {
      return FALSE;
    }

    $settings = $job->getSettings($this->type);
    $skew = $this->getSkew($job);

    $job_last_ran = $log_entry->start_time;
    $time = time();
    $next_schedule = $cron_last_ran = $job_last_ran;
    while ($time > $job_last_ran) {
      foreach ($settings['rules'] as $rule) {
        $next_schedule = $cron_last_ran;
        $cron = CronRule::factory($rule, $time, $skew);
        $cron_last_ran = $cron->getLastSchedule();
        $time = $time > $cron_last_ran - 1 ? $cron_last_ran - 1 : $time;
        if ($time <= $job_last_ran) {
          break;
        }
      }
    }
    $behind = time() - $next_schedule;

    return $next_schedule > $job_last_ran && $behind > $settings['catch_up'] ? $behind: FALSE;
  }

  /**
   * Get a "unique" skew for a job.
   */
  protected function getSkew($job) {
    return isset($this->skews[$job->name]) ? $this->skews[$job->name] : $this->skews[$job->name] = hexdec(substr(sha1($job->name), -2));
  }
}
