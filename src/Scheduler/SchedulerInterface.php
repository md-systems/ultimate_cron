<?php

/**
 * Contains \Drupal\ultimate_cron\Scheduler\SchedulerInterface.
 */

namespace Drupal\ultimate_cron\Scheduler;

use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Component\Plugin\PluginInspectionInterface;
use Drupal\Core\Plugin\PluginFormInterface;

/**
 * Defines a scheduler method.
 */
interface SchedulerInterface extends PluginInspectionInterface, ConfigurablePluginInterface, PluginFormInterface {
}
