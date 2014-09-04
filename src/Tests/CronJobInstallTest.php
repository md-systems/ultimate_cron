<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobInstallTest
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Cron Job Form Testing
 *
 * @group ultimate_cron
 */
class CronJobInstallTest extends WebTestBase {
  public static $modules = array('ultimate_cron');

  /**
   * A user with permission to create and edit books and to administer blocks.
   *
   * @var object
   */
  protected $admin_user;

  /**
   * Tests adding and editing a cron job.
   */
  function testManageJob() {
    // Create user with correct permission.
    $this->admin_user = $this->drupalCreateUser(array('administer ultimate cron'));
    $this->drupalLogin($this->admin_user);

    $this->drupalGet('admin/config/system/cron/jobs');

    $this->assertText('field cronjob title');
    $this->assertText('system cronjob title');
  }
}
