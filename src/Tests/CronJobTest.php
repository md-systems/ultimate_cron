<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobTest.
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\WebTestBase;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * Cron Job Form Testing
 *
 * @group ultimate_cron
 */
class CronJobTest extends WebTestBase {
  public static $modules = array('ultimate_cron');

  /**
   * A user with permission to create and edit books and to administer blocks.
   *
   * @var object
   */
  protected $admin_user;

  /**
   * Cron job name.
   *
   * @var string
   */
  protected $job_name;

  /**
   * Cron job machine id.
   *
   * @var string
   */
  protected $job_id;

  /**
   * Tests adding and editing a cron job.
   */
  function testManageJob() {
    // Create user with correct permission.
    $this->admin_user = $this->drupalCreateUser(array('administer ultimate cron', 'administer site configuration'));
    $this->drupalLogin($this->admin_user);

    // Cron Jobs overview.
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertResponse('200');

    // Start adding a new job.
    $this->clickLink(t('Add job'));
    $this->assertResponse('200');

    // Set new job configuration.
    $this->job_name = 'initial job name';
    $this->job_id = strtolower($this->randomMachineName());
    $job_configuration = array(
      'title' => $this->job_name,
      'id' => $this->job_id,
      'scheduler[settings][rules]' => '604800',
      'launcher[settings][timeouts][lock_timeout]' => '3601',
      'logger[settings][expire]' => '1209601',
    );

    // Save new job.
    $this->drupalPostForm(NULL, $job_configuration, t('Save'));

    // Assert drupal_set_message for successful added job.
    $this->assertText(t('job @name has been added.', array('@name' => $this->job_name)));

    // Assert cron job overview for recently added job.
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertText($this->job_name);

    // Start editing added job.
    $this->drupalGet('admin/config/system/cron/jobs/manage/' . $this->job_id);
    $this->assertResponse('200');

    // Set new cron job configuration and save the old job name.
    $old_job_name = $this->job_name;
    $this->job_name = 'edited job name';
    $edit = array('title' => $this->job_name,);

    // Save the new job.
    $this->drupalPostForm(NULL, $edit, t('Save'));

    // Assert drupal_set_message for successful updated job.
    $this->assertText(t('job @name has been updated.', array('@name' => $this->job_name)));

    //Assert cron job overview for recently updated job.
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertNoText($old_job_name);
    $this->assertText($this->job_name);

    // Check Entity Configuration
    $job_entity = entity_load('ultimate_cron_job', $this->job_id)->toArray();
    $this->assertEqual($job_entity['scheduler']['settings']['rules'], $job_configuration['scheduler[settings][rules]']);
    $this->assertEqual($job_entity['launcher']['settings']['timeouts']['lock_timeout'], $job_configuration['launcher[settings][timeouts][lock_timeout]']);
    $this->assertEqual($job_entity['logger']['settings']['expire'], $job_configuration['logger[settings][expire]']);

    debug(CronJob::load($this->job_id)->loadLatestLogEntry(), 'load latest log entry before');
    $this->drupalPostForm('admin/config/system/cron', array(), t('Run cron'));
    debug(CronJob::load($this->job_id)->loadLatestLogEntry(), 'load latest log entry after');
    // Load Latest Entry
  }
}
