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
    //_ultimate_cron_variable_save('cron_last', time());

    $launcher_jobs = array();
    foreach (CronJob::loadMultiple() as $job) {
      /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
      $manager = \Drupal::service('plugin.manager.ultimate_cron.' . 'launcher');
      //$launcher = $manager->createInstance($job->getLauncherId());


      debug($job->getLauncherId(), $job->getTitle());

//      if (!isset($launchers) || in_array($launcher->title, $launchers)) {
//        $launcher_jobs[$launcher->title]['launcher'] = $launcher;
//        $launcher_jobs[$launcher->title]['sort'] = array($launcher->weight);
//        $launcher_jobs[$launcher->title]['jobs'][$job->getTitle()] = $job;
//        $launcher_jobs[$launcher->title]['jobs'][$job->getTitle()]->sort = array($job->loadLatestLogEntry()->start_time);
//      }
    }

//    uasort($launcher_jobs, '_ultimate_cron_multi_column_sort');
//
//    foreach ($launcher_jobs as $name => $launcher_job) {
//      uasort($launcher_job['jobs'], '_ultimate_cron_multi_column_sort');
//      $launcher_job['launcher']->launchJobs($launcher_job['jobs']);
//    }
    drupal_set_message('Run Ultimate Cron job');
  }
}
