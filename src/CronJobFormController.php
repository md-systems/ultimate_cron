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
    $plugin_types = array('Scheduler', 'Launcher', 'Logger');
    foreach ($plugin_types as $plugin_type) {
      /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
      $manager = \Drupal::service('plugin.manager.ultimate_cron.' . $plugin_type);
      $plugins = $manager->getDefinitions();

      // Generate select options.
      $options = array();
      foreach($plugins as $value => $key) {
        if (!empty($key['default']) && $key['default'] == TRUE) {
          $options = array($value => t('@title (Default)', array('@title' => $key['title']->render()))) + $options;
          $this->selected_option = $value;
        } else {
          $options[$value] = $key['title']->render();
        }
      }

      $form[$plugin_type] = array(
        '#type' => 'details',
        '#title' => t($plugin_type),
        '#group' => 'settings_tabs',
      );

      $form[$plugin_type]['name'] = array(
          '#type' => 'select',
          '#title' => t($plugin_type),
          '#options' => $options,
          '#default_value' => 2,
          '#description' => $this->t("Select which @name to use for this job.", array('@name' => $plugin_type)),
          '#group' => 'settings_tabs',
      );

      // @TODO: Fix this.
      /** @var \Drupal\ultimate_cron\Plugin\ultimate_cron\Scheduler\Simple $instance */
      $instance = $manager->createInstance($this->selected_option);
      $temp_form = array();
      $form[$plugin_type]['settings'] = $instance->settingsForm($temp_form, $form_state);
    }

    $form['#attached']['js'][] = drupal_get_path('module', 'ultimate_cron') . '/js/ultimate_cron.job.js';

    return $form;
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
