<?php

/**
 * Contains \Drupal\ultimate_cron\Logger\LoggerInterface.
 */

namespace Drupal\ultimate_cron\Logger;

use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Component\Plugin\PluginInspectionInterface;
use Drupal\Core\Plugin\PluginFormInterface;

/**
 * Defines a logger method.
 */
interface LoggerInterface extends PluginInspectionInterface, ConfigurablePluginInterface, PluginFormInterface {

}
