<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobFormTest.
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Cron Job Form Testing
 *
 * @group ultimate_cron
 */
class CronJobFormTest extends WebTestBase {
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
  }
}
