<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronDefaultLauncher extends UltimateCronLauncher {
  public function launch($job) {
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
    );
  }

  /**
   * Settings form Ultimate Cron launcher.
   */
  public function settingsForm(&$form, &$form_state) {
  }
}
