<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Tests\SchedulerPluginTest.php
 */

namespace Drupal\ultimate_cron\Tests;

use Drupal\simpletest\DrupalUnitTestBase;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Scheduler\Crontab;
use Drupal\ultimate_cron\Plugin\ultimate_cron\Scheduler\Simple;

class SchedulerPluginTest extends DrupalUnitTestBase {

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
      'name' => 'Scheduler Plugin tests',
      'description' => 'Tests the default scheduler plugins',
      'group' => 'Ultimate Cron',
    );
  }

  /**
   * Tests that scheduler plugins are discovered correctly.
   */
  function testDiscovery() {
    /* @var \Drupal\Core\Plugin\DefaultPluginManager $manager */
    $manager = \Drupal::service('plugin.manager.ultimate_cron.scheduler');

    $plugins = $manager->getDefinitions();
    $this->assertEqual(count($plugins), 2);

    $simple = $manager->createInstance('simple');
    $this->assertTrue($simple instanceof Simple);
    $this->assertEqual($simple->getPluginId(), 'simple');

    $crontab = $manager->createInstance('crontab');
    $this->assertTrue($crontab instanceof Crontab);
    $this->assertEqual($crontab->getPluginId(), 'crontab');
  }
}
