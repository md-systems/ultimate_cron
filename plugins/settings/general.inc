<?php
/**
 * @file
 * Extra settings for cron queues.
 */

$plugin = array(
  'title' => t('General'),
  'description' => t('General cron settings.'),

  'handler' => array(
    'class' => 'UltimateCronGeneralSettings',
    'parent' => 'UltimateCronSettings',
  ),
);
