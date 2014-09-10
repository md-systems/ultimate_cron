<?php

/**
 * @file
 * Contains \Drupal\ultimate_cron\Form\CronJobForm.
 */

namespace Drupal\ultimate_cron\Form;

use Drupal\Core\Entity\EntityForm;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ultimate_cron\CronJobHelper;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * Base form controller for cron job forms.
 */
class CronJobForm extends EntityForm {

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
      '#default_value' => $job->getTitle(),
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
      '#default_value' => $job->getModule(),
    );

    // Setup vertical tabs.
    $form['settings_tabs'] = array(
      '#type' => 'vertical_tabs',
    );

    // Load settings for each plugin in its own vertical tab.
    $plugin_types = CronJobHelper::getPluginTypes();
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

      $form[$plugin_type]['id'] = array(
        '#type' => 'select',
        '#title' => $plugin_label,
        '#options' => $options,
        '#plugin_type' => $plugin_type,
        '#default_value' => $plugin_settings['id'],
        '#description' => $this->t("Select which @plugin to use for this job.", array('@plugin' => $plugin_type)),
        '#group' => 'settings_tabs',
        '#executes_submit_callback' => TRUE,
        '#ajax' => array(
          'callback' => array($this, 'updateSelectedPluginType'),
          'wrapper' => $plugin_type . '_settings',
          'method' => 'replace',
        ),
        '#submit' => array('::submit', '::rebuild'),
        '#limit_validation_errors' => array(array($plugin_type, 'id')),
      );

      $form[$plugin_type]['select'] = array(
        '#type' => 'submit',
        '#name' => $plugin_type . '_select',
        '#value' => t('Select'),
        '#submit' => array('::submit', '::rebuild'),
        '#limit_validation_errors' => array(array($plugin_type, 'id')),
        '#attributes' => array('class' => array('js-hide')),
      );

      // @TODO: Fix this.
      /** @var \Drupal\ultimate_cron\Plugin\ultimate_cron\Scheduler\Simple $instance */
      $plugin = $job->getPlugin($plugin_type);
      $temp_form = array();
      $form[$plugin_type]['settings'] = $plugin->settingsForm($temp_form, $form_state);
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

    $entity->setCallback($entity->getModule() . '_cron');

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
    //@todo: catogery rename to job
    $job = $this->entity;
    $status = $job->save();

    if ($status == SAVED_UPDATED) {
      drupal_set_message(t('job %label has been updated.', array('%label' => $job->label())));
    }
    else {
      drupal_set_message(t('job %label has been added.', array('%label' => $job->label())));
    }

    $form_state['redirect_route']['route_name'] = 'ultimate_cron.job_list';
  }

}
