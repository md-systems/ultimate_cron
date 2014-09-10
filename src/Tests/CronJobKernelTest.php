<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\CronJobKernelTest
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\KernelTestBase;

/**
 * Tests CRUD for cron jobs.
 *
 * @group ultimate_cron
 */
class CronJobKernelTest extends KernelTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('ultimate_cron');

  protected function setup() {
    parent::setUp();

    $this->installSchema('ultimate_cron', 'ultimate_cron_log');
  }

  /**
   * Tests CRUD operations for cron jobs.
   */
  public function testCRUD() {
    $values = array(
      'id' => 'example',
      'title' => $this->randomMachineName(),
      'description' => $this->randomMachineName(),
    );

    /** @var \Drupal\ultimate_cron\Entity\CronJob $cron_job */
    $cron_job = entity_create('ultimate_cron_job', $values);
    $cron_job->save();

    $this->assertEqual($cron_job->id(), 'example');
    $this->assertEqual($cron_job->label(), $values['title']);
    $this->assertTrue($cron_job->status());

    $cron_job->disable();
    $cron_job->save();

    $cron_job = \Drupal::entityManager()->getStorage('ultimate_cron_job')->load('example');
    $this->assertEqual($cron_job->id(), 'example');
    $this->assertFalse($cron_job->status());
  }
}
