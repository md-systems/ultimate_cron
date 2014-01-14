<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronGeneralSettings extends UltimateCronSettings {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'lock_expire' => 3600,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['lock_expire'] = array(
      '#title' => t("Job lock expiration"),
      '#type' => 'textfield',
      '#default_value' => $values['lock_expire'],
      '#description' => t('Number of seconds to keep lock on job.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }
}
