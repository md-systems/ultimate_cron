<?php

/**
 * Contains \Drupal\ultimate_cron\Launcher\LauncherInterface.
 */

namespace Drupal\ultimate_cron\Launcher;

use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Component\Plugin\PluginInspectionInterface;
use Drupal\Core\Plugin\PluginFormInterface;

/**
 * Defines a launcher method.
 */
interface LauncherInterface extends PluginInspectionInterface, ConfigurablePluginInterface, PluginFormInterface {

}
