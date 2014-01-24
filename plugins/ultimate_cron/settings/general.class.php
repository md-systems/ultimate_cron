<?php
/**
 * @file
 * General settings for Ultimate Cron.
 */

/**
 * General settings plugin class.
 */
class UltimateCronGeneralSettings extends UltimateCronSettings {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'poorman' => FALSE,
    );
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!$job) {
      $elements['poorman'] = array(
        '#type' => 'checkbox',
        '#title' => t('Poormans Cron'),
        '#default_value' => $values['poorman'],
        '#description' => t('Enable Poormans Cron'),
        '#fallback' => TRUE,
      );
    }
    else {
      $elements['no_settings'] = array(
        '#markup' => '<p>' . t('This plugin has no settings.') . '</p>',
      );
    }
  }
}
