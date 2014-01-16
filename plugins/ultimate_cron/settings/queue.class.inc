<?php
/**
 * @file
 * Queue settings for Ultimate Cron.
 */

/**
 * Queue settings plugin class.
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
   * Settings form.
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
