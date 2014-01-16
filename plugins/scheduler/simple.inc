<?php
/**
 * @file
 * Simple cron job scheduler for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Simple'),
  'description' => t('Provides a set of predefined intervals for scheduling.'),

  'handler' => array(
    'class' => 'UltimateCronSimpleScheduler',
    'parent' => 'UltimateCronCrontabScheduler',
  ),
);
