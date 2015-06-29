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
 * Form for scheduler settings.
 */
class SchedulerSettingsForm extends GeneralSettingsForm {

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $values = \Drupal::config('ultimate_cron.settings');
    $rules = is_array($values->get('rules')) ? implode(';', $values->get('rules')) : '';

    // Setup vertical tabs.
    $form['settings_tabs'] = array(
      '#type' => 'vertical_tabs',
    );

    // Settings for Crontab.
    $form['Crontab'] = [
      '#type' => 'details',
      '#title' => 'Crontab',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];

    $form['Crontab']['catch_up'] = array(
      '#title' => t("Catch up"),
      '#type' => 'textfield',
      '#default_value' => $values->get('catch_up'),
      '#description' => t("Don't run job after X seconds of rule."),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $form['Crontab']['rules'] = array(
      '#title' => t("Rules"),
      '#type' => 'textfield',
      '#default_value' => $rules,
      '#description' => t('Semi-colon separated list of crontab rules.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#element_validate' => array('ultimate_cron_plugin_crontab_element_validate_rule'),
    );
    $form['Crontab']['rules_help'] = array(
      '#type' => 'fieldset',
      '#title' => t('Rules help'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['Crontab']['rules_help']['info'] = array(
      '#markup' => file_get_contents(drupal_get_path('module', 'ultimate_cron') . '/help/rules.html'),
    );

    // Settings for Simple scheduler.
    $form['Simple'] = [
      '#type' => 'details',
      '#title' => 'Simple',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];

    $options = [
      '* * * * *' => 'Every minute',
      '*/15+@ * * * *' => 'Every 15 minutes',
      '*/30+@ * * * *' => 'Every 30 minutes',
      '0+@ * * * *' => 'Every hour',
      '0+@ */3 * * *' => 'Every 3 hours',
      '0+@ */6 * * *' => 'Every 6 hours',
      '0+@ */12 * * *' => 'Every 12 hours',
      '0+@ 0 * * *' => 'Every day',
      '0+@ 0 * * 0' => 'Every week',
    ];
    $form['Simple']['rule'] = array(
      '#type' => 'select',
      '#title' => t('Run cron every'),
      '#default_value' => $values->get('rule'),
      '#description' => t('Select the interval you wish cron to run on.'),
      '#options' => $options,
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

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
      ->set('catch_up', $form_state->getValue('Crontab')['catch_up'])
      ->set('rules', explode(';', $form_state->getValue('Crontab')['rules']))
      ->set('rule', $form_state->getValue('Simple')['rule'])
      ->save('');

    parent::submitForm($form, $form_state);
  }

}
