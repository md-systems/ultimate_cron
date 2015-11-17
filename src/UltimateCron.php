<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\UltimateCron.
 */

namespace Drupal\ultimate_cron;

use Drupal\Core\Cron;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * The Ultimate Cron service.
 */
class UltimateCron extends Cron {
  /**
   * {@inheritdoc}
   */
  public function run() {

    $launcher_jobs = array();
    foreach (CronJob::loadMultiple() as $job) {
      if ($job->status()) {
        /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
        $manager = \Drupal::service('plugin.manager.ultimate_cron.' . 'launcher');
        $launcher = $manager->createInstance($job->getLauncherId());
        $launcher_definition = $launcher->getPluginDefinition();

        if (!isset($launchers) || in_array($launcher->getPluginId(), $launchers)) {
          $launcher_jobs[$launcher_definition['id']]['launcher'] = $launcher;
          $launcher_jobs[$launcher_definition['id']]['sort'] = array($launcher_definition['weight']);
          $launcher_jobs[$launcher_definition['id']]['jobs'][$job->id()] = $job;
          $launcher_jobs[$launcher_definition['id']]['jobs'][$job->id()]->sort = array($job->loadLatestLogEntry()->start_time);
        }
      }
    }

    uasort($launcher_jobs, '_ultimate_cron_multi_column_sort');

    foreach ($launcher_jobs as $name => $launcher_job) {
      //uasort($launcher_job['jobs'], '_ultimate_cron_multi_column_sort');
      $launcher_job['launcher']->launchJobs($launcher_job['jobs']);
    }

    $this->setCronLastTime();

    return TRUE;
  }
}
