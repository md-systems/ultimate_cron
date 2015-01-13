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

  /**
   * Modules to enable.
   *
   * @var array
   */
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

    // Check default modules
    \Drupal::service('module_installer')->install(array('field'));
    \Drupal::service('module_installer')->install(array('system'));
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertText('Purges deleted Field API data');
    $this->assertText('Cleanup (caches, batch, flood, temp-files, etc.)');
    $this->assertNoText('Deletes temporary files');

    // Install new module.
    \Drupal::service('module_installer')->install(array('file'));
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertText('Deletes temporary files');

    // Uninstall new module.
    \Drupal::service('module_installer')->uninstall(array('file'));
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertNoText('Deletes temporary files');
  }
}
