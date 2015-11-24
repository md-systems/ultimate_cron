<?php

/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\UltimateCronQueueTest.
 *
 * Test that queues are processed on cron using the System module.
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\system\Tests\System\CronQueueTest;

/**
 * Update feeds on cron.
 *
 * @group ultimate_cron
 */
class UltimateCronQueueTest extends CronQueueTest {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('ultimate_cron');
}
