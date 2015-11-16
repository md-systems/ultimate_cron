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
    drupal_set_message(t('Cron job @job_label was successfully run.', ['@job_label' => $ultimate_cron_job->label()]));
    return $this->redirect('entity.ultimate_cron_job.collection');
  }

  /**
   * Discovers new default cron jobs.
   */
  public function discoverJobs() {
    \Drupal::service('ultimate_cron.discovery')->discoverCronJobs();

  }

}
