<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobTest.
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\KernelTestBase;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * Cron Job Form Testing
 *
 * @group ultimate_cron
 */
class CronJobTest extends KernelTestBase {

  public static $modules = array('ultimate_cron');

  /**
   * Tests adding and editing a cron job.
   */
  function testGeneratedJob() {
    $values = array(
      'title' => 'ultimate cron fake cronjob title',
      'id' => 'ultimate_cron_fake_job',
      'module' => 'ultimate_cron_fake',
      'callback' => 'ultimate_cron_fake_cron',
    );

    $job = entity_create('ultimate_cron_job', $values);
    $job->save();



    debug($job->toArray(), 'job to array');

    debug($job->loadLatestLogEntry(), 'load latest log entry');

    // @todo: Tests should not randomly fail on low catch_up value.
    debug(CronJob::load($values['id'])->loadLatestLogEntry(), 'load latest log entry before');
    \Drupal::service('cron')->run();
    $latest_log_entry = CronJob::load($values['id'])->loadLatestLogEntry();
    debug($latest_log_entry, 'load latest log entry after');
    $this->assertNotEqual('0', $latest_log_entry->start_time);

    // Load Latest Entry
  }
}
