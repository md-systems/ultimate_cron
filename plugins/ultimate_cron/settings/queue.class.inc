<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronQueueSettings extends UltimateCronTaggedSettings {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'queue_lease_time' => 30,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['queue_lease_time'] = array(
      '#title' => t("Queue lease time"),
      '#type' => 'textfield',
      '#default_value' => $values['queue_lease_time'],
      '#description' => t('Seconds to claim a cron queue item.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }
}
