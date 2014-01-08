<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Serial'),
  'description' => t('Serial launcher.'),

  'handler' => array(
    'class' => 'UltimateCronSerialLauncher',
    'parent' => 'UltimateCronLauncher',
  ),
);
