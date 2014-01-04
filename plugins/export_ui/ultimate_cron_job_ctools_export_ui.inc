<?php
/**
 * @file
 * Export UI integration.
 */

/**
 * Plugin definition for Ctools Export UI
 */
$plugin = array(
  'schema' => 'ultimate_cron_job',
  'menu' => array(
    #'menu item' => 'cronjobs',
    'menu prefix' => 'admin/config/system/cron',
    'menu item' => 'jobs',
    'menu description' => 'Manage Ultimate Cron jobs',
    /**/
    'items' => array(
      'list callback' => array(
        'type' => MENU_LOCAL_TASK,
        'weight' => 5,
      ),
    ),
    /**/
  ),
  'handler' => array(
    'class' => 'ultimate_cron_job_ctools_export_ui',
    'parent' => 'ctools_export_ui',
  ),

  'title' => t('Cron jobs'),

  'title singular' => t('cron job'),
  'title plural' => t('cron jobs'),
  'title singular proper' => t('Cron job'),
  'title plural proper' => t('Cron jobs'),
);

/**
 * Form to edit the settings of an Ultimate Cron job.
 */
function ultimate_cron_job_ctools_export_ui_form(&$form, &$form_state) {
  dpm($form);
  $form['info']['name'] = array_merge($form['info']['name'], array(
    '#type' => 'value',
  ));
}
