<?php

$plugin = array(
  'title' => t('Database'),
  'description' => t('Stores logs in the database.'),

  'handler' => array(
    'class' => 'UltimateCronDatabaseLogger',
    'parent' => 'UltimateCronLogger',
  ),
);
