<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\LauncherPluginTest.php
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\DrupalUnitTestBase;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Launcher\SerialLauncher;

class LauncherPluginTest extends DrupalUnitTestBase {

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
      'name' => 'Launcher Plugin tests',
      'description' => 'Tests the default scheduler plugins',
      'group' => 'Ultimate Cron',
    );
  }

  /**
   * Tests that scheduler plugins are discovered correctly.
   */
  function testDiscovery() {
    /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
    $manager = \Drupal::service('plugin.manager.ultimate_cron.launcher');

    $plugins = $manager->getDefinitions();
    $this->assertEqual(count($plugins), 1);

    $serial = $manager->createInstance('serial');
    $this->assertTrue($serial instanceof SerialLauncher);
    $this->assertEqual($serial->getPluginId(), 'serial');
  }
}
