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
      'nodejs' => TRUE,
    );
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!$job && module_exists('nodejs')) {
      $elements['nodejs'] = array(
        '#type' => 'checkbox',
        '#title' => t('nodejs'),
        '#default_value' => $values['nodejs'],
        '#description' => t('Enable nodejs integration (live reload on jobs page)'),
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
