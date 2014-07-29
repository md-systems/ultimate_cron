<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\LoggerPluginTest.php
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\KernelTestBase;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Logger\CacheLogger;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Logger\DatabaseLogger;

/**
 * Tests the default scheduler plugins.
 *
 * @group ultimate_cron
 */
class LoggerPluginTest extends KernelTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('ultimate_cron');

  /**
   * Tests that scheduler plugins are discovered correctly.
   */
  function testDiscovery() {
    /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
    $manager = \Drupal::service('plugin.manager.ultimate_cron.logger');

    $plugins = $manager->getDefinitions();
    $this->assertEqual(count($plugins), 2);

    $cache = $manager->createInstance('cache');
    $this->assertTrue($cache instanceof CacheLogger);
    $this->assertEqual($cache->getPluginId(), 'cache');

    $database = $manager->createInstance('database');
    $this->assertTrue($database instanceof DatabaseLogger);
    $this->assertEqual($database->getPluginId(), 'database');
  }
}
