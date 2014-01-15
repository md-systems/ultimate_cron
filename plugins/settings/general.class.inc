<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronGeneralSettings extends UltimateCronSettings {
  public $poorman_intervals = array(
    60,
    300,
    600,
    1800,
    3600,
    10800,
    21600,
    43200,
    86400,
    604800
  );

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'lock_timeout' => 3600,
      'poorman_interval' => 300,
      'poorman_page_flush' => TRUE,
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    if (!$job) {
      $elements['jobs'] = array(
        '#type' => 'fieldset',
        '#title' => t('Jobs'),
      );
    }

    $elements['jobs']['lock_timeout'] = array(
      '#parents' => array('settings', $this->type, $this->name, 'lock_timeout'),
      '#title' => t("Job lock timeout"),
      '#type' => 'textfield',
      '#default_value' => $values['lock_timeout'],
      '#description' => t('Number of seconds to keep lock on job.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    if (!$job) {
      $elements['poorman'] = array(
        '#type' => 'fieldset',
        '#title' => t('Poormans Cron'),
      );
      $options = array(0 => t('Never')) + drupal_map_assoc($this->poorman_intervals, 'format_interval');
      $elements['poorman']['poorman_interval'] = array(
        '#parents' => array('settings', $this->type, $this->name, 'poorman_interval'),
        '#type' => 'select',
        '#title' => t('Poormans Cron interval'),
        '#default_value' => $values['poorman_interval'],
        '#description' => t('Trigger Poormans Cron every'),
        '#options' => $options,
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
      $elements['poorman']['poorman_page_flush'] = array(
        '#type' => 'checkbox',
        '#title' => t('Poormans Cron page flush'),
        '#default_value' => $values['poorman_page_flush'],
        '#description' => t('Perform early page flush to avoid end users waiting for cron run.'),
        '#fallback' => TRUE,
      );
    }
  }
}
