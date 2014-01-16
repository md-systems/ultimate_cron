<?php
/**
 * @file
 * Queue settings for Ultimate Cron.
 */

$plugin = array(
  'title' => t('Queue'),
  'description' => t('Cron queue settings.'),

  'handler' => array(
    'class' => 'UltimateCronQueueSettings',
    'parent' => 'UltimateCronTaggedSettings',
  ),
);
