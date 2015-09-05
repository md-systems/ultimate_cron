<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobFormTest.
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\WebTestBase;
use Drupal\Component\Utility\SafeMarkup;

/**
 * Cron Job Form Testing
 *
 * @group ultimate_cron
 */
class CronJobFormTest extends WebTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('ultimate_cron', 'block');

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
    $this->drupalPlaceBlock('local_tasks_block');
    $this->drupalPlaceBlock('local_actions_block');

    // Create user with correct permission.
    $this->admin_user = $this->drupalCreateUser(array('administer ultimate cron', 'administer site configuration'));
    $this->drupalLogin($this->admin_user);

    // Cron Jobs overview.
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertResponse('200');

    // Check for the default schedule message in Job list.
    $this->assertText('Every 15 min');
    // Check for the Last Run default value.
    $this->assertText('Never');

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
    $edit = array('title' => $this->job_name);

    // Save the new job.
    $this->drupalPostForm(NULL, $edit, t('Save'));
    // Assert the edited Job hasn't run yet.
    $this->assertNoUniqueText('Never');
    // Assert drupal_set_message for successful updated job.
    $this->assertText(t('job @name has been updated.', array('@name' => $this->job_name)));

    // Run the Jobs.
    $this->cronRun();

    // Assert the cron jobs have been run by checking the time.
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertNoUniqueText(SafeMarkup::format('@time', array('@time' => \Drupal::service('date.formatter')->format(\Drupal::state()->get('system.cron_last'), "short"))), "Created Cron jobs have been run.");

    // Check that all jobs have been run.
    $this->assertNoText("Never");

    // Assert cron job overview for recently updated job.
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertNoText($old_job_name);
    $this->assertText($this->job_name);

    // Change time when cron runs, check the 'Scheduled' label is updated.
    $this->clickLink(t('Edit'));
    $this->drupalPostForm(NULL, ['scheduler[configuration][rules][0]' => '0+@ */6 * * *'], t('Save'));
    $this->assertText('Every 6 hours');

    // Test deleting a job.
    $this->clickLink(t('Delete'), 1);
    $this->drupalPostForm(NULL, NULL, t('Delete'));
    $this->assertText('The cron job edited job name has been deleted.');
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertNoText($this->job_name);
    $this->clickLink(t('Add job'));
    $job_configuration = array(
      'title' => 'Test Job',
      'id' => strtolower($this->randomMachineName()),
      'scheduler[id]' => 'crontab',
    );

    // Save new job.
    $this->drupalPostForm(NULL, $job_configuration, t('Save'));
    $this->clickLink(t('Edit'), 1);
    $this->drupalPostForm(NULL, ['scheduler[configuration][rules][0]' => '0+@ * * * *'], t('Save'));
    $this->assertText('Rule: 0+@ * * * *');
  }

}
