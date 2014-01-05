<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

/**
 * Queue scheduler.
 */
class UltimateCronQueueScheduler extends UltimateCronCrontabScheduler {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'rules' => array('* * * * *'),
      'queue_lease_time' => 30,
    ) + parent::defaultSettings();
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    parent::settingsForm($form, $form_state);
    $form['settings']['scheduler_settings']['queue_lease_time'] = array(
      '#title' => t('Queue lease time'),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings']['scheduler_settings']['queue_lease_time'],
      '#description' => t('Time in seconds to keep lock on claimed item'),
      '#fallback' => TRUE,
      '#fallback_required' => TRUE,
    );
  }
}
