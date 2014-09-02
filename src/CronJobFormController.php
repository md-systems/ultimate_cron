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

  /**
   * {@inheritdoc}
   */
  public function form(array $form, FormStateInterface $form_state) {
    $form = parent::form($form, $form_state);
    /* @var \Drupal\ultimate_cron\Entity\CronJob $job */
    $job = $this->entity;

    $form['title'] = array(
      '#title' => t('Title'),
      '#type' => 'textfield',
      '#default_value' => $job->title,
    );
    $form['id'] = array(
      '#type' => 'machine_name',
      '#default_value' => $job->id(),
      '#machine_name' => array(
        'exists' => 'contact_category_load',
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
    $plugin_types = array();array('launcher', 'logger', 'scheduler');
    foreach ($plugin_types as $plugin_type => $info) {
      $static = $info['defaults']['static'];
      $class = $static['class'];
      $class::jobSettingsForm($form, $form_state, $plugin_type, $job);
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
