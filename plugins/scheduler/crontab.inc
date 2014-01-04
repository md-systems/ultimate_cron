<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Crontab'),
  'description' => t('Crontab scheduler.'),

  'handler' => array(
    'class' => 'UltimateCronCrontabScheduler',
  ),
);

/**
 * Pseudo round-robin balancer plugin class.
 */
class UltimateCronCrontabScheduler extends UltimateCronScheduler {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'rules' => '*/10+@ * * * *',
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $form['settings']['scheduler_settings']['rules'] = array(
      '#title' => t("Rules"),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings']['scheduler_settings']['rules'],
      '#description' => t('Comma separated list of crontab rules.'),
      '#fallback' => TRUE,
      '#fallback_required' => TRUE,
      #'#element_validate' => array('element_validate_crontab_rule'),
    );
  }
}
