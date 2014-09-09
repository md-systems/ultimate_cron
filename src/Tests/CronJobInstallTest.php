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
   * @var \Drupal\Core\Extension\ModuleHandler
   */
  protected $moduleHandler;

  /**
   * Sets up the module handler for enabling and disabling modules.
   */
  protected function setUp() {
    parent::setUp();
    $this->moduleHandler = $this->container->get('module_handler');
  }

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
    $this->assertNoText('file cronjob title');

//    // enable core module
    $this->moduleHandler->install(array('file'));
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertText('file cronjob title');

    $this->moduleHandler->uninstall(array('file'));
    $this->drupalGet('admin/config/system/cron/jobs');
    $this->assertNoText('file cronjob title');

    // uninstall it
  }
}
