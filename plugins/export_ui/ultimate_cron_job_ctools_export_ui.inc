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
  $job = $form_state['item'];

  $form['info']['name'] = array_merge($form['info']['name'], array(
    '#type' => 'value',
  ));

  $form['jid'] = array(
    '#type'  => 'value',
    '#value' => isset($job->jid) ? $job->jid : '',
  );


  $schedulers = ultimate_cron_scheduler_load_all();
  $options = array();
  $options[0] = t('Default (@default)', array(
    '@default' => $schedulers[$job->hook['scheduler']]->title,
  ));
  foreach ($schedulers as $name => $scheduler) {
    $options[$name] = $scheduler->title;
  }

  // Sanitize input values.
  if (!isset($form_state['values'])) {
    $form_state['values'] = array();
  }
  if (!isset($form_state['values']['settings'])) {
    $form_state['values']['settings'] = array();
  }
  $form_state['values']['settings'] += array(
    'scheduler' => 0,
    'scheduler_settings' => !empty($job->settings['scheduler_settings']) ? $job->settings['scheduler_settings'] : array(),
  );

  if (empty($form_state['values']['settings']['scheduler'])) {
    $scheduler = $schedulers[$job->hook['scheduler']];
  }
  else {
    $scheduler = $schedulers[$job->settings['scheduler']];
  }

  // If this is an AJAX scheduler change, let's set the original values.
  if (!empty($form_state['triggering_element']) && $form_state['triggering_element']['#name'] == 'scheduler') {
    $form_state['values']['settings']['scheduler_settings'] = $job->settings['scheduler_settings'];
  }


  // Ensure that we at least have some default values.
  $defaults = $job->hook['scheduler_settings'];
  $form_state['default_values']['settings']['scheduler_settings'] = $defaults;
  $form_state['values']['settings']['scheduler_settings'] += ultimate_cron_blank_values($defaults);

  $form['settings'] = array(
    '#tree' => TRUE
  );
  $form['settings']['scheduler'] = array(
    '#type'          => 'select',
    '#options'       => $options,
    '#default_value' => $form_state['values']['settings']['scheduler'],
    '#title'         => t('Scheduler'),
    '#required'      => TRUE,
    '#description'   => t('Scheduler for this job.'),
    '#ajax' => array(
      'callback' => 'ultimate_cron_job_scheduler_settings_ajax',
      'wrapper' => 'scheduler-settings',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );

  $form['settings']['scheduler_settings'] = array(
    '#title' => t("@scheduler scheduler settings", array('@scheduler' => $scheduler->title)),
    '#prefix' => '<div id="scheduler-settings">',
    '#suffix' => '</div>',
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#visible' => TRUE,
  );
  $form['settings']['scheduler_settings']['scheduler'] = array(
    '#type' => 'value',
    '#value' => $scheduler->name,
  );

  $scheduler->settingsForm($form, $form_state);
  ultimate_cron_fallbackalize($form['settings']['scheduler_settings'], $form_state['default_values']['settings']['scheduler_settings'], FALSE);
}

/**
 * Ajax callback for interchangable scheduler settings.
 */
function ultimate_cron_job_scheduler_settings_ajax($form, &$form_state) {
  return $form['settings']['scheduler_settings'];
}

/**
 * Validate submission of the preset edit form.
 */
function ultimate_cron_job_ctools_export_ui_form_validate(&$form, &$form_state) {
  if (!isset($form_state['values']['settings']['scheduler_settings'])) {
    $form_state['values']['settings']['scheduler_settings'] = array();
  }
  $scheduler = !empty($form_state['values']['settings']['scheduler']) ? $form_state['values']['settings']['scheduler'] : variable_get('ultimate_cron_default_scheduler', ULTIMATE_CRON_DEFAULT_SCHEDULER);
  $scheduler = ultimate_cron_scheduler_load($scheduler);
  $scheduler->settingsFormValidate($form, $form_state);
}

/**
 * Submit handler of the preset edit form.
 *
 * The submit handler is only responsible for moving data from
 * $form_state['values'] to $form_state['item'].
 */
function ultimate_cron_job_ctools_export_ui_form_submit(&$form, &$form_state) {
  $scheduler = !empty($form_state['values']['settings']['scheduler']) ? $form_state['values']['settings']['scheduler'] : variable_get('ultimate_cron_default_scheduler', ULTIMATE_CRON_DEFAULT_SCHEDULER);
  $scheduler = ultimate_cron_scheduler_load($scheduler);
  $scheduler->settingsFormSubmit($form, $form_state);
  foreach ($form_state['values']['settings']['scheduler_settings'] as $key => $value) {
    if (
      empty($value) &&
      !empty($form['settings']['scheduler_settings'][$key]['#fallback']) &&
      empty($form['settings']['scheduler_settings'][$key]['#use_fallback_value'])
    ) {
      unset($form_state['values']['settings']['scheduler_settings'][$key]);
    }
  }
}
