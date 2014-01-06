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

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $rules = $this->settingsFormValue('rules', $form_state['values']['settings']['scheduler_settings']['rules']);
    $form['settings']['scheduler_settings']['rules'] = array(
      '#title' => t("Rules"),
      '#type' => 'textfield',
      '#default_value' => $rules,
      '#description' => t('Comma separated list of crontab rules.'),
      '#fallback' => TRUE,
      '#fallback_required' => TRUE,
      #'#element_validate' => array('element_validate_crontab_rule'),
    );
  }

  /**
   * Submit handler.
   */
  public function settingsFormSubmit(&$form, &$form_state) {
    if (!empty($form_state['values']['settings']['scheduler_settings']['rules'])) {
      $rules = explode(',', $form_state['values']['settings']['scheduler_settings']['rules']);
      $form_state['values']['settings']['scheduler_settings']['rules'] = array_map('trim', $rules);
    }
  }

  public function schedule($job) {
    
  }
}
