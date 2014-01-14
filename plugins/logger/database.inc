<?php

$plugin = array(
  'title' => t('Database'),
  'description' => t('Database logger.'),

  'handler' => array(
    'class' => 'UltimateCronDatabaseLogger',
    'parent' => 'UltimateCronLogger',
  ),
);
