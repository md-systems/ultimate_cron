<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\CronJobHelper.
 *
 * Cron Job helper class.
 */
namespace Drupal\ultimate_cron;

use Drupal\ultimate_cron\Entity\CronJob;

class CronJobHelper {

  /**
   * Creates a new cron job with specific values.
   *
   * @param $info
   *   Module name.
   */
  public static function ensureCronJobExists($info, $id) {
    $job = NULL;
    $module_titles = CronJobHelper::getJobTitle();

    if (!CronJob::load($id)) {
      $callback = $info['callback'];
      debug($callback);
      $title = isset($module_titles[$callback]) ? $module_titles[$callback]['title'] : 'Default cron handler';

      $values = array(
        'title' => $title,
        'id' => $id,
        'module' => $info['module'],
        'callback' => $callback,
      );

      $job = CronJob::create($values);

      $job->save();
    }
  }

  public static function getJobTitle() {
    $update = array();

    $update['comment_cron']['title'] = t('Store the maximum possible comments per thread');
    $update['dblog_cron']['title'] = t('Remove expired log messages and flood control events');
    $update['field_cron']['title'] = t('Purges deleted Field API data');
    $update['file_cron']['title'] = t('Deletes temporary files');
    $update['history_cron']['title'] = t('Deletes history');
    $update['search_cron']['title'] = t('Updates indexable active search pages');
    $update['system_cron']['title'] = t('Cleanup (caches, batch, flood, temp-files, etc.)');
    $update['update_cron']['title'] = t('Update indexes');
    $update['node_cron']['title'] = t('Mark old nodes as read');
    $update['aggregator_cron']['title'] = t('Refresh feeds');
    $update['statistics_cron']['title'] = t('Reset counts and clean up');
    $update['tracker_cron']['title'] = t('Update tracker index');

    return $update;
  }


  public static function getPluginTypes() {
    return array(
      'scheduler' => t('Scheduler'),
      'launcher' => t('Launcher'),
      'logger' => t('Logger')
    );
  }
}

