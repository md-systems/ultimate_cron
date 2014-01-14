<?php
/**
 * @file
 * Ultimate Cron!
 */

/**
 * Main plugin settings form.
 */
function ultimate_cron_plugin_form($form, &$form_state, $plugin_type) {
  ctools_include('plugins');
  $plugin_types = ctools_plugin_get_plugin_type_info();
  $plugin_info = $plugin_types['ultimate_cron'][$plugin_type];
  $static = $plugin_info['defaults']['static'];
  $class = $static['class'];
  $class::defaultSettingsForm($form, $form_state, $plugin_info);
  return $form;
}

/**
 * Settings form for plugins.
 */
function ultimate_cron_plugin_settings($form, &$form_state, $plugin_type, $name) {
  $plugin = ultimate_cron_plugin_load($plugin_type, $name);
  $form['description'] = array(
    '#markup' => $plugin->description,
  );

  // Sanitize input values.
  if (!isset($form_state['values']['settings'][$plugin_type][$name])) {
    $form_state['values']['settings'][$plugin_type][$name] = array();
  }
  $form_state['values']['settings'][$plugin_type][$name] += $plugin->getDefaultSettings();
  $form_state['default_values']['settings'][$plugin_type][$name] = $form_state['values']['settings'][$plugin_type][$name];

  $form['settings']['#tree'] = TRUE;
  $form['settings'][$plugin_type]['#tree'] = TRUE;
  $form['plugin_type'] = array(
    '#type' => 'value',
    '#value' => $plugin_type,
  );
  $form['name'] = array(
    '#type' => 'value',
    '#value' => $name,
  );

  $plugin->settingsForm($form, $form_state);

  if (!empty($form['settings'][$plugin_type][$name]['no_settings'])) {
    return $form;
  }

  $plugin->fallbackalize($form['settings'][$plugin_type][$name], $form_state['default_values']['settings'][$plugin_type][$name], TRUE);
  $form = system_settings_form($form);
  $form['#validate'] = empty($form['#validate']) ? array() : $form['#validate'];
  $form['#submit'] = empty($form['#submit']) ? array() : $form['#submit'];
  array_unshift($form['#validate'], 'ultimate_cron_plugin_settings_validate');
  array_unshift($form['#submit'], 'ultimate_cron_plugin_settings_submit');
  return $form;
}

/**
 * Validate handler for plugin settings.
 */
function ultimate_cron_plugin_settings_validate($form, &$form_state) {
  $plugin_type = $form_state['values']['plugin_type'];
  $name = $form_state['values']['name'];
  $plugin = ultimate_cron_plugin_load($plugin_type, $name);
  $plugin->settingsFormValidate($form, $form_state);
}

/**
 * Submit handler for plugin settings.
 */
function ultimate_cron_plugin_settings_submit($form, &$form_state) {
  $plugin_type = $form_state['values']['plugin_type'];
  $name = $form_state['values']['name'];
  $plugin = ultimate_cron_plugin_load($plugin_type, $name);
  $plugin->settingsFormSubmit($form, $form_state);

  // Since this is a system_settings_form() we need to move the submitted data
  // around a little.
  $form_state['values'][$plugin->key] = $form_state['values']['settings'][$plugin_type][$name];
  unset($form_state['values']['plugin_type']);
  unset($form_state['values']['name']);
  unset($form_state['values']['settings']);
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
