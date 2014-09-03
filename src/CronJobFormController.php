<?php

/**
 * @file
 * Contains \Drupal\ultimate_cron\CronJobFormController.
 */

namespace Drupal\ultimate_cron;

use Drupal\Core\Entity\EntityForm;
use Drupal\Core\Form\FormStateInterface;

/**
 * Base form controller for cron job forms.
 */
class CronJobFormController extends EntityForm {

  protected $selected_option;

  /**
   * {@inheritdoc}
   */
  public function form(array $form, FormStateInterface $form_state) {
    $form = parent::form($form, $form_state);
    /* @var \Drupal\ultimate_cron\Entity\CronJob $job */
    $job = $this->entity;

    $form['title'] = array(
      '#title' => t('Title'),
      '#description' => t('This will appear in the administrative interface to easily identify it.'),
      '#type' => 'textfield',
      '#default_value' => $job->title,
    );

    $form['id'] = array(
      '#type' => 'machine_name',
      '#default_value' => $job->id(),
      '#machine_name' => array(
        'exists' => '\Drupal\ultimate_cron\Entity\CronJob::load',
        'source' => array('title'),
      ),
      '#disabled' => !$job->isNew(),
    );

    $options = array();

    foreach(\Drupal::moduleHandler()->getImplementations('cron') as $module_name) {
      $options[$module_name] = $module_name;
    }

    $form['module'] = array(
      '#title' => 'Module',
      '#description' => 'Module name for callback, suffix _cron added.',
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $job->module,
    );

    // Setup vertical tabs.
    $form['settings_tabs'] = array(
      '#type' => 'vertical_tabs',
    );
    $form['settings'] = array(
      '#tree' => TRUE,
    );

    // Base the form on the actual form data, in case of AJAX request.
    if (isset($form_state['input'])) {
      $form_state['values'] = $form_state['input'];
    }

    // Sanitize input values.
    if (!isset($form_state['values']['settings'])) {
      $form_state['values']['settings'] = array();
    }
    $form_state['values']['settings'] += $job->settings;

    // Load settings for each plugin in its own vertical tab.
    $plugin_types = array(
      'scheduler' => t('Scheduler'),
      'launcher' => t('Launcher'),
      'logger' => t('Logger')
    );
    foreach ($plugin_types as $plugin_type => $plugin_label) {
      /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
      $manager = \Drupal::service('plugin.manager.ultimate_cron.' . $plugin_type);
      $plugins = $manager->getDefinitions();

      $plugin_settings = $job->get($plugin_type);

      // Generate select options.
      $options = array();
      foreach ($plugins as $value => $key) {
        if (!empty($key['default']) && $key['default'] == TRUE) {
          $options = array($value => t('@title (Default)', array('@title' => $key['title']->render()))) + $options;
        }
        else {
          $options[$value] = $key['title']->render();
        }
      }

      $form[$plugin_type] = array(
        '#type' => 'details',
        '#title' => $plugin_label,
        '#group' => 'settings_tabs',
        '#tree' => TRUE,
      );

      $form[$plugin_type]['name'] = array(
        '#type' => 'select',
        '#title' => $plugin_label,
        '#options' => $options,
        '#plugin_type' => $plugin_type,
        '#default_value' => $plugin_settings['name'],
        '#description' => $this->t("Select which @name to use for this job.", array('@name' => $plugin_type)),
        '#group' => 'settings_tabs',
        '#executes_submit_callback' => TRUE,
        '#ajax' => array(
          'callback' => array($this, 'updateSelectedPluginType'),
          'wrapper' => $plugin_type . '_settings',
          'method' => 'replace',
        ),
        '#submit' => array('::submit', '::rebuild'),
        '#limit_validation_errors' => array(array($plugin_type, 'name')),
      );

      $form[$plugin_type]['select'] = array(
        '#type' => 'submit',
        '#name' => $plugin_type . '_select',
        '#value' => t('Select'),
        '#submit' => array('::submit', '::rebuild'),
        '#limit_validation_errors' => array(array($plugin_type, 'name')),
        '#attributes' => array('class' => array('js-hide')),
      );

      // @TODO: Fix this.
      /** @var \Drupal\ultimate_cron\Plugin\ultimate_cron\Scheduler\Simple $instance */
      $instance = $manager->createInstance($plugin_settings['name']);
      $temp_form = array();
      $form[$plugin_type]['settings'] = $instance->settingsForm($temp_form, $form_state);
      $form[$plugin_type]['settings']['#prefix'] = '<div id="' . $plugin_type . '_settings' . '">';
      $form[$plugin_type]['settings']['#suffix'] = '</div>';
    }

    //$form['#attached']['js'][] = drupal_get_path('module', 'ultimate_cron') . '/js/ultimate_cron.job.js';

    return $form;
  }

  public function updateSelectedPluginType(array $form, FormStateInterface $form_state) {
    return $form[$form_state['triggering_element']['#plugin_type']]['settings'];
  }

  /**
   * {@inheritdoc}
   */
  public function buildEntity(array $form, FormStateInterface $form_state) {
    $entity = parent::buildEntity($form, $form_state);
    $entity->callback = $entity->module . '_cron';

    return $entity;
  }

  /**
   * {@inheritdoc}
   */
  public function rebuild(array $form, FormStateInterface $form_state) {
    $form_state->setRebuild(TRUE);
  }

  /**
   * Overrides Drupal\Core\Entity\EntityForm::validate().
   */
  public function validate(array $form, FormStateInterface $form_state) {
    parent::validate($form, $form_state);
  }

  /**
   * Overrides Drupal\Core\Entity\EntityForm::save().
   */
  public function save(array $form, FormStateInterface $form_state) {
    $category = $this->entity;
    $status = $category->save();

    if ($status == SAVED_UPDATED) {
      drupal_set_message(t('job %label has been updated.', array('%label' => $category->label())));
    }
    else {
      drupal_set_message(t('job %label has been added.', array('%label' => $category->label())));
    }

    $form_state['redirect_route']['route_name'] = 'ultimate_cron.job_list';
  }

}
