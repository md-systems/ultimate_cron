<?php
/**
 * @file
 * Crontab cron job scheduler for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Crontab'),
  'description' => t('Use crontab rules for scheduling jobs.'),

  'handler' => array(
    'class' => 'UltimateCronCrontabScheduler',
    'parent' => 'UltimateCronScheduler',
  ),
);
