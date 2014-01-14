<?php
/**
 * @file
 * Extra settings for cron queues.
 */

$plugin = array(
  'title' => t('Queue'),
  'description' => t('Cron queue settings.'),

  'handler' => array(
    'class' => 'UltimateCronQueueSettings',
    'parent' => 'UltimateCronTaggedSettings',
  ),
);
