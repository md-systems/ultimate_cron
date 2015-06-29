<?php
/**
 * Created by PhpStorm.
 * User: berdir
 * Date: 4/4/14
 * Time: 3:03 PM
 */

namespace Drupal\ultimate_cron\Form;
use Drupal\Core\Form\FormStateInterface;

/**
 * Form for logger settings.
 */
class LoggerSettingsForm extends GeneralSettingsForm {

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $values = \Drupal::config('ultimate_cron.settings');

    // Setup vertical tabs.
    $form['settings_tabs'] = [
      '#type' => 'vertical_tabs',
    ];

    // Settings for Cache logger.
    $form['Cache'] = [
      '#type' => 'details',
      '#title' => 'Cache',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];

    $form['Cache']['bin'] = array(
      '#type' => 'textfield',
      '#title' => t('Cache bin'),
      '#description' => t('Select which cache bin to use for storing logs.'),
      '#default_value' => $values->get('bin'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $form['Cache']['timeout'] = array(
      '#type' => 'textfield',
      '#title' => t('Cache timeout'),
      '#description' => t('Seconds before cache entry expires (0 = never, -1 = on next general cache wipe).'),
      '#default_value' => $values->get('timeout'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    // Settings for Database logger.
    $form['Database'] = [
      '#type' => 'details',
      '#title' => 'Database',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];
    $options['method'] = [1 => 'Disabled', 2 => 'Remove logs older than a specified age', 3 => 'Retain only a specific amount of log entries'];
    $form['Database']['method'] = array(
      '#type' => 'select',
      '#title' => t('Log entry cleanup method'),
      '#description' => t('Select which method to use for cleaning up logs.'),
      '#options' => $options['method'],
      '#default_value' => $values->get('method'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $states = array('expire' => array(), 'retain' => array());
    $form['Database']['method_expire'] = array(
      '#type' => 'fieldset',
      '#title' => t('Remove logs older than a specified age'),
    ) + $states['expire'];
    $form['Database']['method_expire']['expire'] = array(
      '#type' => 'textfield',
      '#title' => t('Log entry expiration'),
      '#description' => t('Remove log entries older than X seconds.'),
      '#default_value' => $values->get('expire'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    ) + $states['expire'];

    $form['Database']['method_retain'] = array(
      '#type' => 'fieldset',
      '#title' => t('Retain only a specific amount of log entries'),
    ) + $states['retain'];
    $form['Database']['method_retain']['retain'] = array(
      '#type' => 'textfield',
      '#title' => t('Retain logs'),
      '#description' => t('Retain X amount of log entries.'),
      '#default_value' => $values->get('retain'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    ) + $states['retain'];

    $form['actions'] = [
      '#type' => 'actions',
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save configuration',
        '#button_type' => 'primary',
      ]
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->config('ultimate_cron.settings')
      ->set('bin', $form_state->getValue('Cache')['bin'])
      ->set('timeout', $form_state->getValue('Cache')['timeout'])
      ->set('method', $form_state->getValue('Database')['method'])
      ->set('expire', $form_state->getValue('Database')['method_expire']['expire'])
      ->set('retain', $form_state->getValue('Database')['method_retain']['retain'])
      ->save('');

    parent::submitForm($form, $form_state);
  }

}
