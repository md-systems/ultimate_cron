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
 * Form for launcher settings.
 */
class LauncherSettingsForm extends GeneralSettingsForm {

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $values = \Drupal::config('ultimate_cron.settings');

    $form['timeouts'] = [
      '#type' => 'fieldset',
      '#title' => t('Timeouts'),
    ];
    $form['launcher'] = [
      '#type' => 'fieldset',
      '#title' => t('Launching options'),
    ];
    $form['timeouts']['lock_timeout'] = [
      '#parents' => ['settings', 'lock_timeout'],
      '#title' => t("Job lock timeout"),
      '#type' => 'textfield',
      '#default_value' => $values->get('lock_timeout'),
      '#description' => t('Number of seconds to keep lock on job.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    ];
    $form['timeouts']['max_execution_time'] = [
      '#title' => t("Maximum execution time"),
      '#type' => 'textfield',
      '#default_value' => $values->get('max_execution_time'),
      '#description' => t('Maximum execution time for a cron run in seconds.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    ];
    $form['launcher']['max_threads'] = [
      '#parents' => array('settings', 'max_threads'),
      '#title' => t("Maximum number of launcher threads"),
      '#type' => 'textfield',
      '#default_value' => $values->get('max_threads'),
      '#description' => t('The maximum number of launch threads that can be running at any given time.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#weight' => 1,
    ];
    $form['launcher']['poorman_keepalive'] = [
      '#parents' => [
        'settings',
        'poorman_keepalive',
      ],
      '#title' => t("Poormans cron keepalive"),
      '#type' => 'checkbox',
      '#default_value' => $values->get('poorman_keepalive'),
      '#description' => t('Retrigger poormans cron after it has finished. Requires $base_url to be accessible from the webserver.'),
      '#fallback' => TRUE,
      '#weight' => 3,
    ];

    $options = ['any', '-- fixed --', '1'];

    $form['launcher']['thread'] = [
      '#parents' => ['settings', 'thread'],
      '#title' => t("Run in thread"),
      '#type' => 'select',
      '#default_value' => $values->get('thread'),
      '#options' => $options,
      '#description' => t('Which thread to run jobs in.') . "<br/>" .
        t('<strong>Any</strong>: Just use any available thread') . "<br/>" .
        t('<strong>Fixed</strong>: Only run in one specific thread. The maximum number of threads is spread across the jobs.') . "<br/>" .
        t('<strong>1-?</strong>: Only run when a specific thread is invoked. This setting only has an effect when cron is run through cron.php with an argument ?thread=N or through Drush with --options=thread=N.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#weight' => 2,
    ];

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
      ->set('lock_timeout', $form_state->getValue('settings')['lock_timeout'])
      ->set('max_threads', $form_state->getValue('settings')['max_threads'])
      ->set('poorman_keepalive', $form_state->getValue('settings')['poorman_keepalive'])
      ->set('thread', $form_state->getValue('settings')['thread'])
      ->save('');

    parent::submitForm($form, $form_state);
  }

}
