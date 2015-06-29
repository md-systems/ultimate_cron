<?php
/**
 * Created by PhpStorm.
 * User: berdir
 * Date: 4/4/14
 * Time: 3:03 PM
 */

namespace Drupal\ultimate_cron\Form;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\CronInterface;
use Drupal\Core\Datetime\DateFormatter;
use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\State\StateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpFoundation\RedirectResponse;

/**
 * Form for general cron settings.
 */
class GeneralSettingsForm extends ConfigFormBase {

  /**
   * Stores the state storage service.
   *
   * @var \Drupal\Core\State\StateInterface
   */
  protected $state;

  /**
   * The cron service.
   *
   * @var \Drupal\Core\CronInterface
   */
  protected $cron;

  /**
   * The date formatter service.
   *
   * @var \Drupal\Core\Datetime\DateFormatter
   */
  protected $dateFormatter;

  /**
   * Constructs a GerneralSettingsForm object.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The factory for configuration objects.
   * @param \Drupal\Core\State\StateInterface $state
   *   The state key value store.
   * @param \Drupal\Core\CronInterface $cron
   *   The cron service.
   * @param \Drupal\Core\Datetime\DateFormatter $date_formatter
   *   The date formatter service.
   */
  public function __construct(ConfigFactoryInterface $config_factory, StateInterface $state, CronInterface $cron, DateFormatter $date_formatter) {
    parent::__construct($config_factory);
    $this->state = $state;
    $this->cron = $cron;
    $this->dateFormatter = $date_formatter;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('config.factory'),
      $container->get('state'),
      $container->get('cron'),
      $container->get('date.formatter')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'ultimate_cron_settings';
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['ultimate_cron.settings'];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $values = \Drupal::config('ultimate_cron.settings');
    // Setup vertical tabs.
    $form['settings_tabs'] = [
      '#type' => 'vertical_tabs',
    ];

    // General Settings.
    $form['General'] = [
      '#type' => 'details',
      '#title' => 'General',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];

    // Run Cron manually.
    $form['General']['run'] = [
      '#type' => 'submit',
      '#value' => t('Run cron'),
      '#submit' => ['::submitCron'],
    ];

    // Last run time of cron.
    $status = '<p>' . $this->t('Last run: %time ago.', array('%time' => $this->dateFormatter->formatTimeDiffSince($this->state->get('system.cron_last')))) . '</p>';
    $form['General']['status'] = [
      '#markup' => $status,
    ];

    // Print the Cron run URL as a link.
    $form['General']['cron_url'] = array(
      '#markup' => '<p>' . t(
          'To run cron from outside the site, go to <a href="!cron">!cron</a>',
          ['!cron' => $this->url('system.cron', ['key' => $this->state->get('system.cron_key')], ['absolute' => TRUE])]
        ) . '</p>',
    );

    // Configure the interval between cron runs.
    $form['General']['cron'] = array(
      '#title' => t('Cron settings'),
      '#type' => 'details',
      '#open' => TRUE,
    );
    $options = array(3600, 10800, 21600, 43200, 86400, 604800);
    $form['General']['cron']['cron_safe_threshold'] = array(
      '#type' => 'select',
      '#title' => t('Run cron every'),
      '#description' => t('More information about setting up scheduled tasks can be found by <a href="@url">reading the cron tutorial on drupal.org</a>.', array('@url' => 'https://www.drupal.org/cron')),
      '#default_value' => $values->get('threshold_autorun'),
      '#options' => array(0 => t('Never')) + array_map(array($this->dateFormatter, 'formatInterval'), array_combine($options, $options)),
    );

    $form['General']['nodejs'] = array(
      '#type' => 'checkbox',
      '#title' => t('nodejs'),
      '#default_value' => $values->get('nodejs'),
      '#description' => t('Enable nodejs integration (Live reload on jobs page. Requires the nodejs module to be installed and configured).'),
      '#fallback' => TRUE,
    );

    // Poormans settings.
    $form['Poormans'] = [
      '#type' => 'details',
      '#title' => 'Poormans',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];

    $options = ['Serial'];
    $form['Poormans']['launcher'] = array(
      '#type' => 'select',
      '#title' => t('Launcher'),
      '#options' => $options,
      '#default_value' => $values->get('launcher'),
      '#description' => t('Select the launcher to use for handling poormans cron.'),
      '#fallback' => TRUE,
    );
    $form['Poormans']['early_page_flush'] = array(
      '#type' => 'checkbox',
      '#title' => t('Early page flush'),
      '#default_value' => $values->get('early_page_flush'),
      '#description' => t('If not checked, Ultimate Cron will postpone the poormans cron execution until every shutdown function has run.'),
      '#fallback' => TRUE,
    );
    $form['Poormans']['user_agent'] = [
      '#type' => 'textfield',
      '#title' => t('User Agent'),
      '#default_value' => $values->get('user_agent'),
      '#description' => t('The User Agent to use for poormans cron triggering (used by the Serial launcher).'),
      '#fallback' => TRUE,
    ];

    // Queue settings.
    $form['Queue'] = [
      '#type' => 'details',
      '#title' => 'Queue',
      '#group' => 'settings_tabs',
      '#tree' => TRUE,
    ];

    $form['Queue']['enabled'] = array(
      '#title' => t('Enable cron queue processing'),
      '#description' => t('If enabled, cron queues will be processed by this plugin. If another cron queue plugin is installed, it may be necessary/beneficial to disable this plugin.'),
      '#type' => 'checkbox',
      '#default_value' => $values->get('queue_enabled', TRUE),
      '#fallback' => TRUE,
    );


    $states = array(
      '#states' => array(
        'visible' => array(':input[name="settings[Queue][throttle]"]' => array('checked' => TRUE)),
      ),
    );

    $form['Queue']['timeouts'] = array(
        '#type' => 'fieldset',
        '#title' => t('Timeouts'),
      ) + $states;
    $form['Queue']['timeouts']['lease_time'] = array(
      '#title' => t("Queue lease time"),
      '#type' => 'textfield',
      '#default_value' => $values->get('lease_time'),
      '#description' => t('Seconds to claim a cron queue item.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $form['Queue']['timeouts']['time'] = array(
      '#title' => t('Time'),
      '#type' => 'textfield',
      '#default_value' => $values->get('time'),
      '#description' => t('Time in seconds to process items during a cron run.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $form['Queue']['delays'] = array(
        '#type' => 'fieldset',
        '#title' => t('Delays'),
      ) + $states;
    $form['Queue']['delays']['empty_delay'] = array(
      '#title' => t("Empty delay"),
      '#type' => 'textfield',
      '#default_value' => $values->get('empty_delay'),
      '#description' => t('Seconds to delay processing of queue if queue is empty (0 = end job).'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $form['Queue']['delays']['item_delay'] = array(
      '#title' => t("Item delay"),
      '#type' => 'textfield',
      '#default_value' => $values->get('item_delay'),
      '#description' => t('Seconds to wait between processing each item in a queue.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $form['Queue']['throttle'] = array(
      '#title' => t('Throttle'),
      '#type' => 'checkbox',
      '#default_value' => $values->get('throttle'),
      '#description' => t('Throttle queues using multiple threads.'),
    );

    $form['Queue']['throttling'] = array(
        '#type' => 'fieldset',
        '#title' => t('Throttling'),
      ) + $states;
    $form['Queue']['throttling']['threads'] = array(
      '#title' => t('Threads'),
      '#type' => 'textfield',
      '#default_value' => $values->get('threads'),
      '#description' => t('Number of threads to use for queues.'),
      '#states' => array(
        'visible' => array(':input[name="settings[Queue][throttle]"]' => array('checked' => TRUE)),
      ),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $form['Queue']['throttling']['threshold'] = array(
      '#title' => t('Threshold'),
      '#type' => 'textfield',
      '#default_value' => $values->get('threshold'),
      '#description' => t('Number of items in queue required to activate the next cron job.'),
      '#states' => array(
        'visible' => array(':input[name="settings[Queue][throttle]"]' => array('checked' => TRUE)),
      ),
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
      ->set('threshold_autorun', $form_state->getValue('General')['cron']['cron_safe_threshold'])
      ->set('launcher', $form_state->getValue('Poormans')['launcher'])
      ->set('early_page_flush', $form_state->getValue('Poormans')['early_page_flush'])
      ->set('user_agent', $form_state->getValue('Poormans')['user_agent'])
      ->set('enabled', $form_state->getValue('Queue')['enabled'])
      ->set('lease_time', $form_state->getValue('Queue')['timeouts']['lease_time'])
      ->set('time', $form_state->getValue('Queue')['timeouts']['time'])
      ->set('empty_delay', $form_state->getValue('Queue')['delays']['empty_delay'])
      ->set('item_delay', $form_state->getValue('Queue')['delays']['item_delay'])
      ->set('throttle', $form_state->getValue('Queue')['throttle'])
      ->set('threads', $form_state->getValue('Queue')['throttling']['threads'])
      ->set('threshold', $form_state->getValue('Queue')['throttling']['threshold'])
      ->save('');

    parent::submitForm($form, $form_state);
  }

  /**
   * Runs cron and reloads the page.
   *
   * @param array $form
   *   The form which started the Cron runs.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The form_state of the submitting form.
   *
   * @return RedirectResponse
   *   Return to the settings page.
   */
  public function submitCron(array &$form, FormStateInterface $form_state) {
    // Run cron manually from Cron form.
    if ($this->cron->run()) {
      drupal_set_message(t('Cron run successfully.'));
    }
    else {
      drupal_set_message(t('Cron run failed.'), 'error');
    }

    return new RedirectResponse($this->url('ultimate_cron.settings', array(), array('absolute' => TRUE)));
  }

}
