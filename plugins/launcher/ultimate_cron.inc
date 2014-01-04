<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Ultimate Cron'),
  'description' => t('Ultimate Cron launcher.'),

  'handler' => array(
    'class' => 'UltimateCronDefaultLauncher',
  ),
);

/**
 * Pseudo round-robin balancer plugin class.
 */
class UltimateCronDefaultLauncher extends UltimateCronLauncher {
  public function launch() {
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
    );
  }

  /**
   * Settings form for HTTP dispatcher.
   */
  public function settingsForm(&$form, &$form_state) {
  }
}
