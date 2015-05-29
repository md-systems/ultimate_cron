<?php

/**
 * Contains \Drupal\ultimate_cron\Scheduler\SchedulerInterface.
 */

namespace Drupal\ultimate_cron\Scheduler;

use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Component\Plugin\PluginInspectionInterface;
use Drupal\Core\Plugin\PluginFormInterface;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * Defines a scheduler method.
 */
interface SchedulerInterface extends PluginInspectionInterface, ConfigurablePluginInterface, PluginFormInterface {

  /**
   * Returns the default configuration.
   *
   * @return mixed
   */
  public function defaultConfiguration();
  /**
   * Label for schedule.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job whose label should be formatted.
   */
  public function formatLabel(CronJob $job);

  /**
   * Label for schedule.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job whose label should be formatted.
   */
  public function formatLabelVerbose(CronJob $job);

  /**
   * Settings form for the scheduler.
   *
   * @param array &$form
   *   The form.
   * @param array $form_state
   *   The form's state.
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job for which the settings form applies.
   */
  public function settingsForm(array &$form, array &$form_state, CronJob $job = NULL);

  /**
   * Submit handler.
   *
   * @param array &$form
   *   The form.
   * @param array $form_state
   *   The form's state.
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job for which the settings form applies.
   */
  public function settingsFormSubmit(array &$form, array &$form_state, CronJob $job = NULL);

  /**
   * Check job schedule.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job to check schedule for.
   *
   * @return bool
   *   TRUE if job is scheduled to run.
   */
  public function isScheduled(CronJob $job);

  /**
   * Check if job is behind schedule.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $job
   *   The job to check schedule for.
   *
   * @return bool
   *   TRUE if job is behind its schedule.
   */
  public function isBehind(CronJob $job);

}
