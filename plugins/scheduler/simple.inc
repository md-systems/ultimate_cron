<?php
/**
 * @file
 * Simple cron job scheduler for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Simple'),
  'description' => t('Simple scheduler.'),

  'handler' => array(
    'class' => 'UltimateCronSimpleScheduler',
    'parent' => 'UltimateCronCrontabScheduler',
  ),
);
