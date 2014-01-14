<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

/**
 * Queue scheduler.
 */
class UltimateCronSimpleScheduler extends UltimateCronCrontabScheduler {
  public $hasSettings = TRUE;

  public $presets = array(
    '* * * * *' => 60,
    '0 * * * *' => 3600,
    '0 */3 * * *' => 10800,
    '0 */6 * * *' => 21600,
    '0 */12 * * *' => 43200,
    '0 0 * * *' => 86400,
    '0 0 * * 0' => 604800,
  );

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'rules' => array('0 * * * *'),
    ) + parent::defaultSettings();
  }

  /**
   * Form value for settings.
   */
  public function settingsFormValue($name, $value) {
    switch ($name) {
      case 'rules':
        return is_array($value) ? reset($value) : '';
    }
    return parent::settingsFormValue($name, $value);
  }

  /**
   * Label for settings.
   */
  public function settingsLabel($name, $value) {
    switch ($name) {
      case 'rules':
        return isset($value[0]) ? format_interval($this->presets[$value[0]]) : $value;
    }
    return parent::settingsLabel($name, $value);
  }

  public function getScheduledLabel($settings) {
    return isset($settings['rules'][0]) ? t('Every @interval', array('@interval' => format_interval($this->presets[$settings['rules'][0]]))) : t('N/A');
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
    $rule = $this->settingsFormValue('rules', $values['rules']);

    $intervals = drupal_map_assoc($this->presets, 'format_interval');
    $options = array_combine(array_keys($this->presets), $intervals);

    $elements['rules'] = array(
      '#type' => 'select',
      '#title' => t('Run cron every'),
      '#default_value' => $rule,
      '#description' => t('Select the interval you wish cron to run on.'),
      '#options' => $options,
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }
}
