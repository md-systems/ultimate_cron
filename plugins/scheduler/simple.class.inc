<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

/**
 * Queue scheduler.
 */
class UltimateCronSimpleScheduler extends UltimateCronCrontabScheduler {
  public $presets = array(
    '* * * * *' => 60,
    '0 * * * *' => 3600,
    '0 */3 * * *' => 10800,
    '0 */6 * * *' => 21600,
    '* */12 * * *' => 43200,
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
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
    $rule = reset($form_state['values']['settings']['scheduler_settings']['rules']);
    $default_rule = reset($form_state['default_values']['settings']['scheduler_settings']['rules']);
    $form_state['default_values']['settings']['scheduler_settings']['rules'] = format_interval($this->presets[$default_rule]);

    $options = array();
    $options[0] = t('Default (@default)', array(
      '@default' => format_interval($this->presets[$default_rule]),
    ));

    $intervals = drupal_map_assoc($this->presets, 'format_interval');
    $options += array_combine(array_keys($this->presets), $intervals);


    $form['settings']['scheduler_settings']['rules'] = array(
      '#type' => 'select',
      '#title' => t('Run cron every'),
      '#default_value' => $rule,
      '#description' => t('Time in seconds to keep lock on claimed item'),
      '#options' => $options,
      '#fallback' => TRUE,
      '#fallback_required' => TRUE,
    );
  }
}
