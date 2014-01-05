<?php
/**
 * @file
 * Default cron job scheduler for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Queue'),
  'description' => t('Queue scheduler.'),

  'handler' => array(
    'class' => 'UltimateCronQueueScheduler',
    'parent' => 'UltimateCronCrontabScheduler',
  ),
);
