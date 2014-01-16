<?php
/**
 * @file
 * General settings for Ultimate Cron.
 */

$plugin = array(
  'title' => t('General'),
  'description' => t('General cron settings.'),

  'handler' => array(
    'class' => 'UltimateCronGeneralSettings',
    'parent' => 'UltimateCronSettings',
  ),
);
