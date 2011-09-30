<?php

/**
 * Menu callback: runs cron and returns to status-report page.
 */
function ultimate_cron_run_cron() {
  // Run cron manually
  if (ultimate_cron_cron(TRUE)) {
    // drupal_set_message(t('Ultimate Cron started.'));
  }
  else {
    // drupal_set_message(t('Ultimate Cron could not start.'), 'error');
  }
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
  $fid = ultimate_cron_get_function_id($function);
  $conf = db_select('ultimate_cron_configuration', 'ucc')
    ->fields('ucc', array('fid', 'configuration'))
    ->condition('fid', $fid)
    ->execute()
    ->fetchObject();
  $conf = $conf && $conf->configuration ? unserialize($conf->configuration) : array();
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
    '#description' => t('Comma separated list of rules for this job. (blank = ' . variable_get('ultimate_cron_rule', ULTIMATE_CRON_RULE) . ')'),
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
  
  $modules = array();
  foreach ($hooks as $function => $hook) {
    if (!$module || $module == $hook['module']) {
      ultimate_cron_load_hook_data($hook);
      $hook['background_process'] = background_process_get_process('ultimate_cron:' . $hook['fid']);
      $modules[$hook['module']][$function] = $hook;
    }
  }

  $headers = array('', t('Module'), t('Function'), t('Rules'), t('Start'), t('Duration'), t('Status'), array('colspan' => 4, 'data' => ''));
  $output = '';
  $rows = array();
  
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
        $log['start'] = $process->start;
        $log['end'] = microtime(TRUE);
      }



      $link_configure = '';
      if (!empty($hook['configure'])) {
        $link_configure = l('Configure', $hook['configure'], array('attributes' => array('class' => 'module-link module-link-configure')));
      }

      $link_unlock = '';
      if ($process) {
        $link_unlock = l('Unlock', 'background-process/unlock/' . $process->handle, array('query' => array('destination' => trim(request_uri(), '/'))));
      }

      $link_settings = l('Settings', 'admin/config/system/cron/settings/' . $function, array('attributes' => array('class' => 'module-link module-link-configure'), 'query' => array('destination' => trim(request_uri(), '/'))));
      $link_execute = l('Run', 'admin/ultimate-cron/service/start/' . $function, array('attributes' => array('class' => 'module-link'), 'query' => array('destination' => trim(request_uri(), '/'))));
      $link_log = l('Log', 'admin/reports/cron/' . $function, array('attributes' => array('class' => 'module-link')));
      
      if (!empty($conf) && empty($conf['enabled'])) {
        $link_toggle = l('Enable', 'admin/ultimate-cron/service/enable/' . $function, array('attributes' => array('class' => 'module-link module-link-configure'), 'query' => array('destination' => trim(request_uri(), '/'))));
      }
      else {
        $link_toggle = l('Disable', 'admin/ultimate-cron/service/disable/' . $function, array('attributes' => array('class' => 'module-link module-link-configure'), 'query' => array('destination' => trim(request_uri(), '/'))));
      }

      $data = array(
        array('class' => 'ultimate-cron-admin-enable'),
        array('class' => 'ultimate-cron-admin-module'),
        array('class' => 'ultimate-cron-admin-function'),
        array('class' => 'ultimate-cron-admin-rules'),
        array('class' => 'ultimate-cron-admin-start'),
        array('class' => 'ultimate-cron-admin-end'),
        array('class' => 'ultimate-cron-admin-status ultimate-cron-admin-status-' . $css_status),
        array('class' => 'ultimate-cron-admin-settings'),
        array('class' => 'ultimate-cron-admin-configure'),
        array('class' => 'ultimate-cron-admin-log'),
        array('class' => 'ultimate-cron-admin-execute'),
      );
      $data[0]['data'] = $link_toggle;
      $data[1]['data'] = ultimate_cron_module_name($module);
      $data[2]['data'] = $hook['description'];
      $data[3]['data'] = join("<br/>", $rules);
      $data[4]['data'] = $log['start'] ? format_date((int)$log['start'], 'custom', 'Y-m-d H:i:s') : ($process ? t('Starting') : t('Never'));
      $data[5]['data'] = $log['start'] ? gmdate('H:i:s', (int)($log['end'] - $log['start'])) : t('N/A');
//            'title' => t('Finished @ !timestamp', array('!timestamp' => format_date((int)$log['end'], 'custom', 'Y-m-d H:i:s'))),
      if ($process) {
        $data[6]['data'] = '<span>' . t('Running') . '</span>';
        $data[6]['title'] = t('Running on @host', array('@host' => $service_host));
      }
      else {
        $data[6]['data'] = '<span>' . $short_msg . '</span>';
        $data[6]['title'] = strip_tags($msg);
      }
      $data[7]['data'] = $link_settings;
      $data[8]['data'] = $link_configure;
      $data[9]['data'] = $link_log;
      $data[10]['data'] = $hook['unsafe'] ? '' : ($process ? $link_unlock : $link_execute);

      $rows[] = $data;
    }
  }
  $output .= theme('table', array(
    'header' => $headers, 
    'rows' => $rows, 
    'attributes' => array('id' => 'ultimate-cron-view')
  ));

  if (!empty($unsafe)) {
    $msg = t('Functions not handled by Ultimate Cron due to module weight: %functions', array('%functions' => join(', ', $unsafe)));
    drupal_set_message($msg, 'warning');
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
