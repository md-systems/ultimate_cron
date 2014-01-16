<?php

$plugin = array(
  'title' => t('Cache'),
  'description' => t('Stores the last log entry (and only the last) in the cache.'),

  'handler' => array(
    'class' => 'UltimateCronCacheLogger',
    'parent' => 'UltimateCronLogger',
  ),
);
