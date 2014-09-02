<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobsFormTest.
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Token integration.
 *
 * @group Currency
 */
class CronJobsFormTest extends WebTestBase {
  public static $modules = array('ultimate_cron');

  /**
   * A user with permission to create and edit books and to administer blocks.
   *
   * @var object
   */
  protected $admin_user;

  protected function setUp() {
    parent::setUp();

    // Create user with correct permission.
    $this->admin_user = $this->drupalCreateUser(array('administer ultimate cron'));
    $this->drupalLogin($this->admin_user);
  }

  /**
   * Tests adding a new cron job.
   */
  function testAddJob() {
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertResponse('200');
  }
}
