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
   * @param array $values
   *   Values equal to schema.
   * @return array/boolean
   */
  public static function createCronJob($values) {
    $job = CronJob::create($values);

    $job->save();

    return $job;
  }


  public static function getJobTitle() {
    $update = array();
      $update['dblog_cron']['title'] = t('Remove expired log messages and flood control events');
      $update['field_cron']['title'] = t('Purges deleted Field API data');
      $update['filter_cron']['title'] = t('Expire outdated filter cache entries');
      $update['node_cron']['title'] = t('Mark old nodes as read');
      $update['search_cron']['title'] = t('Update indexes');
      $update['system_cron']['title'] = t('Cleanup (caches, batch, flood, temp-files, etc.)');
      $update['aggregator_cron']['title'] = t('Refresh feeds');
      $update['openid_cron']['title'] = t('Remove expired nonces from the database');
      $update['ping_cron']['title'] = t('Notify remote sites');
      $update['poll_cron']['title'] = t('Close expired polls');
      $update['statistics_cron']['title'] = t('Reset counts and clean up');
      $update['trigger_cron']['title'] = t('Run actions for cron triggers');
      $update['tracker_cron']['title'] = t('Update tracker index');
      $update['update_cron']['title'] = t('Check system for updates');
      $update['dblog_cron']['configure'] = 'admin/config/development/logging';
      $update['ctools_cron']['title'] = t('Clean up old caches');

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

