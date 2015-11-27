<?php

/**
 * @file
 * Contains \Drupal\ultimate_cron\Controller\JobController.
 */

namespace Drupal\ultimate_cron\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * A controller to interact with CronJob entities.
 */
class JobController extends ControllerBase {

  /**
   * Runs a single cron job.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $ultimate_cron_job
   *   The cron job which will be run.
   *
   * @return \Symfony\Component\HttpFoundation\RedirectResponse
   *   Redirects to the job listing after running a job.
   */
  public function runCronJob(CronJob $ultimate_cron_job) {
    $ultimate_cron_job->run();
    drupal_set_message($this->t('Cron job @job_label was successfully run.', ['@job_label' => $ultimate_cron_job->label()]));
    return $this->redirect('entity.ultimate_cron_job.collection');
  }

  /**
   * Discovers new default cron jobs.
   */
  public function discoverJobs() {
    \Drupal::service('ultimate_cron.discovery')->discoverCronJobs();
    drupal_set_message($this->t('Completed discovery for now cron jobs.'));
    return $this->redirect('entity.ultimate_cron_job.collection');
  }

  /**
   * Displays a detailed cron job logs table.
   *
   * @param \Drupal\ultimate_cron\Entity\CronJob $ultimate_cron_job
   *   The cron job which will be run.
   *
   * @return array
   *   A render array as expected by drupal_render().
   */
  public function showLogs(CronJob $ultimate_cron_job) {

    $rows = array();
    $header = array(
      $this->t('Severity'),
      $this->t('User'),
      $this->t('Name'),
      $this->t('Lid'),
      $this->t('Start Time'),
      $this->t('End Time'),
      $this->t('Init Message'),
      $this->t('Message'),
      $this->t('Duration'),
    );

    $log_entries = $ultimate_cron_job->getLogEntries();
    //debug($log_entries->getData());
    foreach ($log_entries as $log_entry) {
      $row = array();
      $row[] = array(
        'data' => array(
          '#type' => 'item',
          '#title' => $log_entry->formatSeverity()[0]['#theme'],
        ),
      );
      $row[] = $log_entry->formatUser();
      $row[] = $log_entry->name;
      $row[] = $log_entry->lid;
      $row[] = $log_entry->formatStartTime();
      $row[] = $log_entry->formatEndTime();
      $row[] = $log_entry->formatInitMessage();
      $row[] = $log_entry->message;
      $row[] = $log_entry->formatDuration();

      $rows[] = $row;
    }
    $form['ultimate_cron_job_logs_table'] = array(
      '#type' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#empty' => $this->t('No log information available.'),
      '#weight' => 120,
    );
    return $form;

  }

}
