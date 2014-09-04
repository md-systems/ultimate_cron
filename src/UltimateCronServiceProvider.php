<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\UltimateCronServiceProvider.
 */

namespace Drupal\ultimate_cron;

use Drupal\Core\DependencyInjection\ContainerBuilder;
use Drupal\Core\DependencyInjection\ServiceProviderBase;

/**
 * Service Provider for File entity.
 */
class UltimateCronServiceProvider extends ServiceProviderBase {

  /**
   * {@inheritdoc}
   */
  public function alter(ContainerBuilder $container) {
    $definition = $container->getDefinition('cron');
    $definition->setClass('Drupal\ultimate_cron\UltimateCron');
  }
}
