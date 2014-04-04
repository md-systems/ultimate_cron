<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\LoggerPluginTest.php
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\DrupalUnitTestBase;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Logger\CacheLogger;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Logger\DatabaseLogger;

class LoggerPluginTest extends DrupalUnitTestBase {

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
      'name' => 'Logger Plugin tests',
      'description' => 'Tests the default scheduler plugins',
      'group' => 'Ultimate Cron',
    );
  }

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
