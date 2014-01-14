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
      'global1' => 10,
      'global2' => '',
      'global3' => 'SSDFSDF',
      'local1' => 10,
      'local2' => '',
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    if (!$job) {
      $form['settings'][$this->type][$this->name]['global3'] = array(
        '#title' => t("My global only required value"),
        '#type' => 'textfield',
        '#default_value' => $form_state['values']['settings'][$this->type][$this->name]['global3'],
        '#description' => t('My global only required value.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
    }

    $form['settings'][$this->type][$this->name]['global1'] = array(
      '#title' => t("My globally required value"),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings'][$this->type][$this->name]['global1'],
      '#description' => t('My globally required value.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $form['settings'][$this->type][$this->name]['global2'] = array(
      '#title' => t("My globally optional value"),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings'][$this->type][$this->name]['global2'],
      '#description' => t('My globally optional value.'),
      '#fallback' => TRUE,
      '#required' => FALSE,
    );
    $form['settings'][$this->type][$this->name]['local1'] = array(
      '#title' => t("My locally required value"),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings'][$this->type][$this->name]['local1'],
      '#description' => t('My locally required value.'),
      '#fallback' => FALSE,
      '#required' => TRUE,
    );
    $form['settings'][$this->type][$this->name]['local2'] = array(
      '#title' => t("My locally optional value"),
      '#type' => 'textfield',
      '#default_value' => $form_state['values']['settings'][$this->type][$this->name]['local2'],
      '#description' => t('My locally optional value.'),
      '#fallback' => FALSE,
      '#required' => FALSE,
    );
  }
}
