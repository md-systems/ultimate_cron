<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\LauncherPluginTest.php
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\DrupalUnitTestBase;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Launcher\SerialLauncher;

class CronJobUnitTest extends DrupalUnitTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('ultimate_cron');

  /**
   * {@inheritdoc}
   */
  public static function getInfo() {
    return array(
      'name' => 'Cron Job tests',
      'description' => 'Tests CRUD for cron jobs',
      'group' => 'Ultimate Cron',
    );
  }

  /**
   * Tests CRUD operations for cron jobs.
   */
  function testCRUD() {
    $values = array(
      'id' => 'example',
      'title' => $this->randomName(),
      'description' => $this->randomName(),
      'settings' => array(
        'example' => $this->randomName(),
      ),
    );
    $cron_job = entity_create('ultimate_cron_job', $values);
    $cron_job->save();

    $this->assertEqual($cron_job->id(), 'example');
    $this->assertEqual($cron_job->label(), $values['title']);
    $this->assertTrue($cron_job->status());

    $cron_job->disable();
    $cron_job->save();

    $cron_job = \Drupal::entityManager()->getStorage('ultimate_cron_job')->load('example');
    $this->assertEqual($cron_job->id(), 'example');
    $this->assertEqual($cron_job->settings, $values['settings']);
    $this->assertFalse($cron_job->status());
  }
}
