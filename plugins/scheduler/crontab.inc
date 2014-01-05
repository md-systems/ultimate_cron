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
