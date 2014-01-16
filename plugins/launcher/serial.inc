<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Serial'),
  'description' => t('Launches scheduled jobs in the same thread and runs them consecutively.'),

  'handler' => array(
    'class' => 'UltimateCronSerialLauncher',
    'parent' => 'UltimateCronLauncher',
  ),
);
