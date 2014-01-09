<?php
/**
 * @file
 * Ultimate Cron!
 */

/**
 * Settings form.
 */
function ultimate_cron_settings_form() {
  $form = array();
  $advanced_help = module_exists('advanced_help');

  $form['queues'] = array(
    '#title' => t('Queues'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['queues']['ultimate_cron_queue_lease_time'] = array(
    '#title' => t('Queue lease time'),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_queue_lease_time', ULTIMATE_CRON_QUEUE_LEASE_TIME),
    '#description' => ($advanced_help ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'queue_lease_time',
                              'type' => 'icon')
                      ) : '') . t('Time in seconds to keep lock on claimed item'),
  );

  $form = system_settings_form($form);
  return $form;
}

/**
 * Main launcher settings form.
 */
function ultimate_cron_launchers_form() {
  $form = array();
  $items = array();
  foreach (ultimate_cron_launcher_load_all() as $name => $launcher) {
    $items[] = l($launcher->title, 'admin/config/system/cron/launchers/' . $name);
  }
  $form['installed'] = array(
    '#markup' => theme('item_list', array(
      'title' => 'Launchers installed',
      'items' => $items
    ))
  );
  return $form;
}

/**
 * Settings form for launchers.
 */
function ultimate_cron_launcher_settings($form, &$form_state, $name) {
  $launcher = ultimate_cron_launcher_load($name);

  $form['description'] = array(
    '#markup' => $launcher->description,
  );

  // Sanitize input values.
  if (!isset($form_state['values'])) {
    $form_state['values'] = array();
  }
  if (!isset($form_state['values']['settings'])) {
    $form_state['values']['settings'] = array();
  }
  $form_state['values']['settings'] += array(
    'launcher_settings' => $launcher->getDefaultSettings(),
  );

  $form_state['default_values']['settings']['launcher_settings'] = $form_state['values']['settings']['launcher_settings'];

  $launcher->settingsForm($form, $form_state);
  if (!empty($form['launcher_settings']['no_settings'])) {
    return $form;
  }
  $form['settings']['#tree'] = TRUE;
  $form['settings']['launcher_settings']['#tree'] = TRUE;
  $form['settings']['launcher'] = array(
    '#type' => 'value',
    '#value' => $name,
  );
  $launcher->fallbackalize($form['settings']['launcher_settings'], $form_state['default_values']['settings']['launcher_settings'], TRUE);
  $form = system_settings_form($form);
  $form['#validate'] = empty($form['#validate']) ? array() : $form['#validate'];
  $form['#submit'] = empty($form['#submit']) ? array() : $form['#submit'];
  array_unshift($form['#validate'], 'ultimate_cron_launcher_settings_validate');
  array_unshift($form['#submit'], 'ultimate_cron_launcher_settings_submit');
  return $form;
}

/**
 * Validate handler for launcher settings.
 */
function ultimate_cron_launcher_settings_validate($form, &$form_state) {
  $launcher = ultimate_cron_launcher_load($form_state['values']['settings']['launcher']);
  $launcher->settingsFormValidate($form, $form_state);
}

/**
 * Submit handler for launcher settings.
 */
function ultimate_cron_launcher_settings_submit($form, &$form_state) {
  $launcher = ultimate_cron_launcher_load($form_state['values']['settings']['launcher']);
  $launcher->settingsFormSubmit($form, $form_state);

  // Since this is a system_settings_form() we need to move the submitted data
  // around a little.
  $form_state['values'][$launcher->key] = $form_state['values']['settings']['launcher_settings'];
  unset($form_state['values']['settings']['launcher']);
  unset($form_state['values']['settings']['launcher_settings']);
}

/**
 * Main launcher settings form.
 */
function ultimate_cron_schedulers_form() {
  $form = array();
  $items = array();
  foreach (ultimate_cron_scheduler_load_all() as $name => $scheduler) {
    $items[] = l($scheduler->title, 'admin/config/system/cron/schedulers/' . $name);
  }
  $form['installed'] = array(
    '#markup' => theme('item_list', array(
      'title' => 'Schedulers installed',
      'items' => $items
    ))
  );
  return $form;
}

/**
 * Settings form for launchers.
 */
function ultimate_cron_scheduler_settings($form, &$form_state, $name) {
  $scheduler = ultimate_cron_scheduler_load($name);

  $form['description'] = array(
    '#markup' => $scheduler->description,
  );

  // Sanitize input values.
  if (!isset($form_state['values'])) {
    $form_state['values'] = array();
  }
  if (!isset($form_state['values']['settings'])) {
    $form_state['values']['settings'] = array();
  }
  $form_state['values']['settings'] += array(
    'scheduler_settings' => $scheduler->getDefaultSettings(),
  );

  $form_state['default_values']['settings']['scheduler_settings'] = $form_state['values']['settings']['scheduler_settings'];

  $scheduler->settingsForm($form, $form_state);
  if (!empty($form['scheduler_settings']['no_settings'])) {
    return $form;
  }
  $form['settings']['#tree'] = TRUE;
  $form['settings']['scheduler_settings']['#tree'] = TRUE;
  $form['settings']['scheduler'] = array(
    '#type' => 'value',
    '#value' => $name,
  );
  $scheduler->fallbackalize($form['settings']['scheduler_settings'], $form_state['default_values']['settings']['scheduler_settings'], TRUE);
  $form = system_settings_form($form);
  $form['#validate'] = empty($form['#validate']) ? array() : $form['#validate'];
  $form['#submit'] = empty($form['#submit']) ? array() : $form['#submit'];
  array_unshift($form['#validate'], 'ultimate_cron_scheduler_settings_validate');
  array_unshift($form['#submit'], 'ultimate_cron_scheduler_settings_submit');
  return $form;
}

/**
 * Validate handler for scheduler settings.
 */
function ultimate_cron_scheduler_settings_validate($form, &$form_state) {
  $scheduler = ultimate_cron_scheduler_load($form_state['values']['settings']['scheduler']);
  $scheduler->settingsFormValidate($form, $form_state);
}

/**
 * Submit handler for scheduler settings.
 */
function ultimate_cron_scheduler_settings_submit($form, &$form_state) {
  $scheduler = ultimate_cron_scheduler_load($form_state['values']['settings']['scheduler']);
  $scheduler->settingsFormSubmit($form, $form_state);

  // Since this is a system_settings_form() we need to move the submitted data
  // around a little.
  $form_state['values'][$scheduler->key] = $form_state['values']['settings']['scheduler_settings'];
  unset($form_state['values']['settings']['scheduler']);
  unset($form_state['values']['settings']['scheduler_settings']);
}

/**
 * Job log page.
 */
function ultimate_cron_function_log_page($name) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[(string)$name])) {
    drupal_not_found();
    exit;
  }
  $hook = $hooks[(string)$name];

  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');

  $header = array(
    array('data' => t('Start'), 'field' => 'start_time', 'sort' => 'DESC'),
    array('data' => t('End'),   'field' => 'end_time'),
    t('Duration'),
    t('Service host'),
    t('Status'),
    t('Message'),
  );

  drupal_set_title(check_plain($name));

  $query = db_select('ultimate_cron_log', 'l');
  $query = $query->condition('l.name', $name)
    ->extend('PagerDefault')
    ->limit(10)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->fields('l', array('lid', 'name', 'start_time', 'end_time', 'service_host', 'severity', 'msg'));
  $logs = $query->execute()->fetchAll(PDO::FETCH_ASSOC);

  $output = '';
  $rows = array();
  if ($hook['background_process']) {
    $process = $hook['background_process'];
    $data = array(
      array('class' => array('ultimate-cron-admin-start')),
      array('class' => array('ultimate-cron-admin-end')),
      array('class' => array('ultimate-cron-admin-duration')),
      array('class' => array('ultimate-cron-admin-status ultimate-cron-admin-status-running')),
      array('class' => array('ultimate-cron-admin-message')),
    );
    $duration = time() - $process->getStartTime();
    $duration = gmdate('H:i:s', (int)$duration);
    if ($process->getProgress() > 0) {
      $duration .= sprintf(" (%3d%%)", $process->getProgress() * 100);
    }
    $data[0]['data'] = format_date((int)$process->getStartTime(), 'custom', 'Y-m-d H:i:s');
    $data[1]['data'] = t('N/A');
    $data[2]['data'] = $duration;
    $data[3]['data'] = $process->getServiceHost() ? $process->getServiceHost() : t('N/A');
    $data[4]['data'] = '<span>' . t('running') . '</span>';
    $data[5]['data'] = '';
    $rows[] = $data;
  }

  $chart = NULL;
  if (module_exists('chart')) {
    $chart = array(
      '#chart_id' => 'ultimate_cron_log_chart_' . $hook['name'],
      '#title' => $hook['description'],
      '#type' => CHART_TYPE_BAR_V_GROUPED,
      '#size' => chart_size(400, 200),
      #'#grid_lines' => chart_grid_lines(10, 15),
      #'#bar_size' => chart_bar_size(25, 25), 
      '#labels' => array(t('Time')),
      '#adjust_resolution' => TRUE,
    );
  }

  foreach ($logs as $log) {
    $severity_type = $log['severity'] < 0 ? 'success' : ($log['severity'] >= WATCHDOG_INFO ? 'info' : ($log['severity'] >= WATCHDOG_WARNING ? 'warning' : 'error'));
    $css_status = $log['msg'] && $severity_type == 'success' ? 'info' : $severity_type;
    $data = array(
      array('class' => array('ultimate-cron-admin-start')),
      array('class' => array('ultimate-cron-admin-end')),
      array('class' => array('ultimate-cron-admin-duration')),
      array('class' => array('ultimate-cron-admin-service-host')),
      array('class' => array('ultimate-cron-admin-status ultimate-cron-admin-status-' . $css_status)),
      array('class' => array('ultimate-cron-admin-message')),
    );
    $data[0]['data'] = format_date((int)$log['start_time'], 'custom', 'Y-m-d H:i:s');
    $data[1]['data'] = format_date((int)$log['end_time'], 'custom', 'Y-m-d H:i:s');
    $data[2]['data'] = gmdate('H:i:s', (int)($log['end_time'] - $log['start_time']));
    $data[3]['data'] = $log['service_host'] ? $log['service_host'] : t('N/A');
    $data[4]['data'] = '<span>' . $log['severity'] . '</span>';
    $data[5]['data'] = $log['msg'];
    $rows[] = $data;

    if ($chart) {
      #$chart['#data'][$data[0]['data']] = ($log['end_time'] - $log['start_time']);
      $chart['#data']['time'][] = ($log['end_time'] - $log['start_time']);
    }
  }

  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'ultimate-cron-view')
  ));
  $output .= theme('pager');


  if ($chart) {
    $output = theme('chart', array('chart' => $chart)) . $output;
  }

  return $output;
}

/**
 * Run a single function.
 *
 * @param $name
 * @return string
 *   Output to page
 */
function ultimate_cron_service_start($name = NULL) {
  dpm("HERE");
  return 'AND HERE';
  if (!isset($name)) {
    $dest = NULL;
    if (empty($_GET['destination'])) {
      $dest = empty($_SERVER['HTTP_REFERER']) ? 'admin/reports/status' : $_SERVER['HTTP_REFERER'];
    }
    ultimate_cron_cron_run(TRUE);
    drupal_goto($dest);
  }

  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[(string)$name])) {
    drupal_not_found();
    exit;
  }

  try {
    $process = ultimate_cron_run_hook($hooks[$name], TRUE);
    drupal_set_message(t('%function started', array('%function' => $name)));
  }
  catch (Exception $e) {
    drupal_set_message(t('%function could not start (%error)', array('%function' => $name, '%error' => $e->getMessage())), 'error');
  }

  drupal_goto();
}

/**
 * Enable/disable cron job
 * @param type $name
 * @param type $enabled
 */
function ultimate_cron_service_enable($name, $enabled) {
  $settings = ultimate_cron_load($name);
  if (!$settings) {
    $settings = new stdClass();
    $settings->name = $name;
  }
  $settings->settings = !empty($settings->settings) ? $settings->settings : array();
  $settings->settings['enabled'] = $enabled;
  ultimate_cron_save($settings);
  drupal_set_message(t('!name !enabled', array('!name' => $name, '!enabled' => t($enabled ? 'enabled' : 'disabled'))));
  drupal_goto();
}

/**
 */
function ultimate_cron_service_process_status() {
  $processes = array();
  $query = db_select('background_process', 'b', array('target' => 'background_process'))
    ->fields('b')
    ->condition('handle', 'uc:%', 'LIKE');
  foreach ($query->execute()->fetchAllAssoc('handle', PDO::FETCH_OBJ) as $process) {
    $process = BackgroundProcess::create($process);
    $name = preg_replace('/^uc:/', '', $process->getHandle());
    $processes[$name] = $process;
    $process->sendMessage('ultimateCronStatus');
  }
  return drupal_json_output(array('processes' => $processes));
}
