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
    'parent' => 'UltimateCronLauncher',
  ),
);
