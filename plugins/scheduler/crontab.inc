<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Crontab'),
  'description' => t('Crontab scheduler.'),

  'handler' => array(
    'class' => 'UltimateCronCrontabScheduler',
  ),
);

/**
 * Pseudo round-robin balancer plugin class.
 */
class UltimateCronCrontabScheduler extends UltimateCronScheduler {
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
    );
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state) {
  }
}
