<?php

/**
 * Menu callback: runs cron and returns to status-report page.
 */
function ultimate_cron_run_cron() {
  // Run the cron and return
  ultimate_cron_cron(TRUE);
  drupal_goto('admin/reports/status');
}

function ultimate_cron_settings_form() {
  $form = array();

   // General settings -----------------------------------
  $form['general'] = array(
    '#title' => t('General'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['general']['ultimate_cron_simultaneous_connections'] = array(
    '#title' => t("Simultaneous connections"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_simultaneous_connections', ULTIMATE_CRON_SIMULTANEOUS_CONNECTIONS),
    '#description' => t('Maximum number of simultaneous connections')
  );
  $form['general']['ultimate_cron_rule'] = array(
    '#title' => t("Default rule"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_rule', ULTIMATE_CRON_RULE),
    '#description' => t('Enter the default fallback rule')
  );
  $form['general']['ultimate_cron_cleanup_log'] = array(
    '#title' => t("Clean up logs older than X seconds"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_cleanup_log', ULTIMATE_CRON_CLEANUP_LOG),
    '#description' => t('Enter maximum age, in seconds, for log entries')
  );
  $form['general']['ultimate_cron_poorman'] = array(
    '#title' => t("Poormans cron"),
    '#type' => 'checkbox',
    '#default_value' => variable_get('ultimate_cron_poorman', ULTIMATE_CRON_POORMAN),
    '#description' => t('Keep background process alive, checking for cron every minute.'),
  );
  $form['general']['ultimate_cron_queue_polling_latency'] = array(
    '#title' => t("Queue polling latency"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_queue_polling_latency', ULTIMATE_CRON_QUEUE_POLLING_LATENCY),
    '#description' => t('Queue polling latency in micro seconds. Leave blank to disable continuous processing of queues.'),
  );
  
  $form = system_settings_form($form);
  
  // Add reclaim button and make sure all the buttons are show last.
  $form['buttons']['#weight'] = 1000;
  $form['buttons']['reclaim'] = array(
    '#value' => t("Reclaim position"),
    '#description' => t('Changes weight of module to the lowest possible.'),
    '#type' => 'submit',
    '#submit' => array('ultimate_cron_reclaim_submit'),
    '#disabled' => !ultimate_cron_uncronable_modules(),
  );

  return $form;
}

function ultimate_cron_reclaim_submit($form, &$form_state) {
  ultimate_cron_reclaim();
  drupal_set_message(t('Module position reclaimed.'));
}

function ultimate_cron_function_settings_form($form, &$form_state, $function) {
  // Load configuration
  $hooks = ultimate_cron_get_hooks();
  $hook = $hooks[$function];
  $fid = $hook['fid'];
  $conf = ultimate_cron_get_configuration($function);
  $conf += array(
    'enabled' => TRUE,
    'rules' => array(),
    'catch_up' => '',
  );
  $form_state['storage']['configuration'] = $conf;

  // Setup form
  drupal_set_title($function);
  $form = array();

   // General settings -----------------------------------
  $form['function'] = array(
    '#type' => 'value',
    '#value' => $function,
  );
  $form['general'] = array(
    '#title' => t('General'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['general']['enabled'] = array(
    '#title' => t('Enabled'),
    '#type' => 'checkbox',
    '#default_value' => $conf['enabled'],
    '#description' => t('Enable this cron job.'),
  );
  $form['general']['rules'] = array(
    '#title' => t('Rules'),
    '#type' => 'textfield',
    '#default_value' => implode(';', $conf['rules']),
    '#description' => t('Semi-colon separated list of rules for this job. (blank = ' . implode(';', $hook['rules']) . ')'),
  );
  $form['general']['catch_up'] = array(
    '#title' => t('Catch up'),
    '#type' => 'textfield',
    '#default_value' => $conf['catch_up'],
    '#description' => t('Run this job if a run was missed. (blank = ' . variable_get('ultimate_cron_catch_up', ULTIMATE_CRON_CATCH_UP) . ')'),
  );

  $options = variable_get('background_process_service_groups', array());
  foreach ($options as $key => &$value) {
    $value = (empty($value['description']) ? $key : $value['description']) . ' (' . join(',', $value['hosts']) . ')';
  }
  $options += array(NULL => 'none');
  $form['general']['service_group'] = array(
    '#type' => 'select',
    '#title' => t('Service group'),
    '#description' => t('Service group to use for this job. See Background Process !url for managing service groups.', array('!url' => l(t('settings'), 'admin/config/background-process'))),
    '#options' => $options,
    '#default_value' => isset($conf['service_group']) ? $conf['service_group'] : NULL,
  );

  $form['buttons'] = array(
    '#weight' => 1000,
  );
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  $form['#redirect'] = 'admin/config/system/cron';

  return $form;
}

function ultimate_cron_function_settings_form_validate($form, &$form_state) {
  $conf =& $form_state['values'];
  $conf['rules'] = trim($conf['rules']);
  $conf['rules'] = $conf['rules'] ? explode(';', $conf['rules']) : array();
  
  if ($conf['rules']) {
    foreach ($conf['rules'] as &$rule) {
      $rule = trim($rule);
      if (!ultimate_cron_validate_rule($rule)) {
        form_set_error('rules', t('Invalid rule.'));
      }
    }
  }
  else {
    unset($conf['rules']);
  }
}

function ultimate_cron_function_settings_form_submit($form, &$form_state) {
  ultimate_cron_set_configuration($form_state['values']['function'], $form_state['values']);
  unset($form_state['storage']);
}

function ultimate_cron_view_page($module = NULL) {
  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');

  $hooks = ultimate_cron_get_hooks();
  $data = _ultimate_cron_preload_cron_data();
  
  $modules = array();
  foreach ($hooks as $function => $hook) {
    if (!$module || $module == $hook['module']) {
      $hook['configuration'] = $data[$function]['configuration'];
      $hook['background_process'] = $data[$function]['background_process'];
      ultimate_cron_load_hook_data($hook);
      $modules[$hook['module']][$function] = $hook;
    }
  }

  $headers = array('', t('Module'), t('Function'), t('Rules'), t('Start'), t('Duration'), t('Status'), array('colspan' => 4, 'data' => ''));
  $output = '';
  $rows = array();

  $overview = array();
  $overview['running'] = 0;
  $overview['errors'] = 0;

  $unsafe = array();
  foreach ($modules as $module => $hooks) {
    foreach ($hooks as $function => $hook) {
      // Collect unsafe functions for message to user
      if ($hook['unsafe']) {
        $unsafe[] = $function;
      }
      
      // Setup configration
      $conf = $hook['configuration'];
      $rules = empty($conf['rules']) ? $hook['rules'] : $conf['rules'];

      // Setup process
      $process = $hook['background_process'];
      $service_host = empty($process->service_host) ? t('N/A') : $process->service_host;

      // Setup log
      $log = $hook['log'];
      if (!$log) {
        $log = array(
          'status' => NULL,
          'start' => NULL,
          'end' => NULL,
        );
      }
      $short_msg = isset($log['status']) ? ($log['status'] ? t('No errors') : t('Errors')) : t('No information');
      $css_status = !empty($process) ? 'running' : (isset($log['status']) ? ($log['status'] ? 'noerror' : 'error') : 'noinfo');
      $msg = !empty($log['msg']) ? $log['msg'] : ($log['status'] ? t('No errors') : t('No information'));
      if ($process) {
        $overview['running']++;
        $log['start'] = $process->start;
        $log['end'] = microtime(TRUE);
      }

      if (isset($log['status']) && !$log['status']) {
        $overview['errors']++;
      }
      $link_configure = '';
      if (!empty($hook['configure'])) {
        $link_configure = _ultimate_cron_l('Module settings', $hook['configure']);
      }

      $link_unlock = '';
      if ($process) {
        $link_unlock = _ultimate_cron_l('Unlock', 'background-process/unlock/' . $process->handle);
      }

      $link_settings = _ultimate_cron_l('Cron settings', 'admin/config/system/cron/settings/' . $function);
      $link_execute = _ultimate_cron_l('Run', 'admin/ultimate-cron/service/start/' . $function);
      $link_log = _ultimate_cron_l('Log', 'admin/reports/cron/' . $function);
      
      $enable = !empty($conf) && empty($conf['enabled']);
      $link_toggle = _ultimate_cron_l($enable ? 'Enable' : 'Disable','admin/ultimate-cron/service/' . ($enable ? 'enable' : 'disable') . '/' . $function);

      $data = array(
        array('class' => $enable ? 'ultimate-cron-admin-enable' : 'ultimate-cron-admin-disable'),
        array('class' => 'ultimate-cron-admin-module'),
        array('class' => 'ultimate-cron-admin-function'),
        array('class' => 'ultimate-cron-admin-rules'),
        array('class' => 'ultimate-cron-admin-start'),
        array('class' => 'ultimate-cron-admin-end'),
        array('class' => 'ultimate-cron-admin-status ultimate-cron-admin-status-' . $css_status),
        array('class' => 'ultimate-cron-admin-settings'),
        array('class' => 'ultimate-cron-admin-configure'),
        array('class' => 'ultimate-cron-admin-log'),
        array('class' => $process ? 'ultimate-cron-admin-unlock' : 'ultimate-cron-admin-execute'),
      );
      $data[0]['data'] = $link_toggle;
      $data[0]['title'] = $enable ? t('Enable') : t('Disable');
      $data[1]['data'] = ultimate_cron_module_name($module);
      $data[2]['data'] = $hook['description'];
      $data[2]['title'] = $function;
      $data[3]['data'] = join("<br/>", $rules);
      $data[4]['data'] = $log['start'] ? format_date((int)$log['start'], 'custom', 'Y-m-d H:i:s') : ($process ? t('Starting') : t('Never'));
      $data[5]['data'] = $log['start'] ? gmdate('H:i:s', (int)($log['end'] - $log['start'])) : t('N/A');
      $data[5]['title'] = t('Finished @ !timestamp', array(
        '!timestamp' => $log['end'] ? format_date((int)$log['end'], 'custom', 'Y-m-d H:i:s') : t('N/A')
      ));
      if ($process) {
        $data[6]['data'] = '<span>' . t('Running') . '</span>';
        $data[6]['title'] = t('Running on @host', array('@host' => $service_host));
      }
      else {
        $data[6]['data'] = '<span>' . $short_msg . '</span>';
        $data[6]['title'] = strip_tags(html_entity_decode($msg, ENT_QUOTES));
      }
      $data[7]['data'] = $link_settings;
      $data[7]['title'] = t('Cron settings');
      $data[8]['data'] = $link_configure;
      $data[8]['title'] = $link_configure ? t('Module settings') : '';
      $data[9]['data'] = $link_log;
      $data[9]['title'] = t('Log');
      $data[10]['data'] = $hook['unsafe'] ? '' : ($process ? $link_unlock : $link_execute);
      $data[10]['title'] = $hook['unsafe'] ? '' : ($process ? t('Unlock') : t('Run'));

      $rows[] = $data;
    }
  }
  $output .= theme('table', array(
    'header' => $headers, 
    'rows' => $rows, 
    'attributes' => array('id' => 'ultimate-cron-view')
  ));

  if (!empty($unsafe)) {
    $msg = t('Functions not handled by Ultimate Cron due to module weight, when cron is invoked through system crontab:<br/>%functions', array('%functions' => join(', ', $unsafe)));
    drupal_set_message($msg, 'warning');
  }
  if ($overview['running']) {
    drupal_set_message(t('@jobs jobs are currently running', array('@jobs' => $overview['running'])));
  }
  if ($overview['errors']) {
    drupal_set_message(t('@jobs jobs failed their last run', array('@jobs' => $overview['errors'])), 'error');
  }

  return $output;
}

function ultimate_cron_function_log_page($function) {
  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');

  $header = array(
    array('data' => t('Start'), 'field' => 'start', 'sort' => 'DESC'),
    array('data' => t('End'),   'field' => 'end'),
    t('Duration'),
    t('Status'),
    t('Message'),
  );

  $query = db_select('ultimate_cron_log', 'l');
  $query->join('ultimate_cron_function', 'f', 'l.fid = f.fid');
  $query = $query->condition('f.function', $function)
             ->extend('PagerDefault')
               ->limit(10)
             ->extend('TableSort')
               ->orderByHeader($header)
             ->fields('l', array('lid', 'fid', 'start', 'end', 'status', 'msg'))
             ->orderBy('l.start', 'DESC');
  $logs = $query->execute()->fetchAll();

  $output = '';
  $rows = array();
  foreach ($logs as $log) {
    $css_status = isset($log->status) ? ($log->status ? 'noerror' : 'error') : 'noinfo';
    $data = array(
      array('class' => array('ultimate-cron-admin-start')),
      array('class' => array('ultimate-cron-admin-end')),
      array('class' => array('ultimate-cron-admin-duration')),
      array('class' => array('ultimate-cron-admin-status ultimate-cron-admin-status-' . $css_status)),
      array('class' => array('ultimate-cron-admin-message')),
    );
    $data[0]['data'] = format_date((int)$log->start, 'custom', 'Y-m-d H:i:s');
    $data[1]['data'] = format_date((int)$log->end, 'custom', 'Y-m-d H:i:s');
    $data[2]['data'] = gmdate('H:i:s', (int)($log->end - $log->start));
    $data[3]['data'] = '<span>' . $log->status . '</span>';
    $data[4]['data'] = $log->msg;
    $rows[] = $data;
  }

  $output .= theme('table', array(
    'header' => $header, 
    'rows' => $rows, 
    'attributes' => array('id' => 'ultimate-cron-view')
  ));
  $output .= theme('pager');

  return $output;
}

/**
 * Run a single function.
 *
 * @param $function
 * @return string
 *   Output to page
 */
function ultimate_cron_service_start($function) {
  $hooks = ultimate_cron_get_hooks();

  if (!isset($hooks[$function])) {
    return drupal_not_found();
  }

  // When run manually don't double check the rules
  $hooks[$function]['skip_catch_up'] = TRUE;

  $handle = ultimate_cron_run_hook($function, $hooks[$function]);

  if ($handle === FALSE) {
    drupal_set_message(t('%function could not start (already running?)', array('%function' => $function)), 'error');
  }
  elseif ($handle === NULL) {
    drupal_set_message(t('%function could not start (service unavailable)', array('%function' => $function)), 'error');
  }
  else {
    drupal_set_message(t('%function started', array('%function' => $function)));
  }

  drupal_goto();
}

/**
 * Enable/disable cron job
 * @param type $function
 * @param type $enabled 
 */
function ultimate_cron_service_enable($function, $enabled) {
  $conf = ultimate_cron_get_configuration($function);
  $conf['enabled'] = $enabled;
  ultimate_cron_set_configuration($function, $conf);
  drupal_goto();
}

/**
 * Helper function for links on cron list
 * @param $text
 *   Text for link
 * @param $path
 *   Path to link to
 * @return type 
 */
function _ultimate_cron_l($text, $path) {
  return l(
    '<span>' . t($text) . '</span>',
    $path,
    array(
      'query' => array('destination' => trim(request_uri(), '/')),
      'html' => TRUE,
    )
  );
}
