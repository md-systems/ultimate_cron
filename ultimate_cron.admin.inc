<?php
/**
 * @file
 */

/**
 * Menu callback: runs cron and returns to status-report page.
 */
function ultimate_cron_run_cron() {
  // Run the cron and return
  ultimate_cron_cron(TRUE);
  drupal_goto('admin/reports/status');
}

/**
 * Settings form.
 */
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
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'simultaneous_connections',
                              'type' => 'icon')
                      ) . t('Maximum number of simultaneous connections'),
  );
  $form['general']['ultimate_cron_rule'] = array(
    '#title' => t("Default rule"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_rule', ULTIMATE_CRON_RULE),
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'rules',
                              'type' => 'icon')
                      ) . t('Enter the default fallback rule'),
  );
  $form['general']['ultimate_cron_cleanup_log'] = array(
    '#title' => t("Clean up logs older than X seconds"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_cleanup_log', ULTIMATE_CRON_CLEANUP_LOG),
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'cleanup_log',
                              'type' => 'icon')
                      ) . t('Enter maximum age, in seconds, for log entries'),
  );
  $form['general']['ultimate_cron_catch_up'] = array(
    '#title' => t('Default catch up'),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_catch_up', ULTIMATE_CRON_CATCH_UP),
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'catch_up',
                              'type' => 'icon')
                      ) . t('Time in seconds to catch up, if a job could not be run within its time frame. (blank = ' . variable_get('ultimate_cron_catch_up', ULTIMATE_CRON_CATCH_UP) . ')'),
  );
  $form['general']['ultimate_cron_queue_polling_latency'] = array(
    '#title' => t("Queue polling latency"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_queue_polling_latency', ULTIMATE_CRON_QUEUE_POLLING_LATENCY),
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'polling_latency',
                              'type' => 'icon')
                      ) . t('Queue polling latency in miliseconds. Leave blank to disable continuous processing of queues.'),
  );
  $form['general']['ultimate_cron_poorman'] = array(
    '#title' => t("Poormans cron"),
    '#type' => 'checkbox',
    '#default_value' => variable_get('ultimate_cron_poorman', ULTIMATE_CRON_POORMAN),
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'poorman',
                              'type' => 'icon')
                      ) . t('Keep background process alive, checking for cron every minute.'),
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

/**
 * Submit handler for reclaiming lowest module weight.
 */
function ultimate_cron_reclaim_submit($form, &$form_state) {
  ultimate_cron_reclaim();
  drupal_set_message(t('Module position reclaimed.'));
}

/**
 * Function settings form.
 */
function ultimate_cron_function_settings_form($form, &$form_state, $function) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[(string)$function])) {
    drupal_not_found();
    exit;
  }

  // Load settings
  $hook = $hooks[$function];
  $conf = ultimate_cron_get_settings($function);
  $conf += _ultimate_cron_default_settings();

  // Setup form
  drupal_set_title(check_plain($function));
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
    '#tree' => TRUE,
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
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'rules',
                              'type' => 'icon')
                      ) . t('Semi-colon separated list of rules for this job. (blank = ' . implode(';', $hook['settings']['rules']) . ')'),
  );
  $form['general']['catch_up'] = array(
    '#title' => t('Catch up'),
    '#type' => 'textfield',
    '#default_value' => $conf['catch_up'],
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'catch_up',
                              'type' => 'icon')
                      ) . t('Time in seconds to catch up, if a job could not be run within its time frame. (blank = ' . variable_get('ultimate_cron_catch_up', ULTIMATE_CRON_CATCH_UP) . ')'),
  );

  $options = variable_get('background_process_service_groups', array());
  foreach ($options as $key => &$value) {
    $value = (empty($value['description']) ? $key : $value['description']) . ' (' . join(',', $value['hosts']) . ')';
  }
  $options += array(NULL => '<default>');
  $form['general']['service_group'] = array(
    '#type' => 'select',
    '#title' => t('Service group'),
    '#description' => theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'service_group',
                              'type' => 'icon')
                      ) . t('Service group to use for this job. See Background Process !url for managing service groups.', array('!url' => l(t('settings'), 'admin/config/system/background-process'))),
    '#options' => $options,
    '#default_value' => isset($conf['service_group']) ? $conf['service_group'] : NULL,
  );

  $form['buttons'] = array(
    '#weight' => 1000,
  );
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );
  $form['#redirect'] = 'admin/config/system/cron';

  return $form;
}

/**
 * Validate handler for function settings.
 */
function ultimate_cron_function_settings_form_validate($form, &$form_state) {
  $conf =& $form_state['values']['general'];
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

/**
 * Submit handler for function settings.
 */
function ultimate_cron_function_settings_form_submit($form, &$form_state) {
  $conf =& $form_state['values']['general'];
  ultimate_cron_set_settings($form_state['values']['function'], $conf);
  unset($form_state['storage']);
}

/**
 * Page overviewing cron jobs.
 */
function ultimate_cron_view_page($module = NULL) {
  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');

  module_load_install('ultimate_cron');
  $requirements = ultimate_cron_requirements('runtime');
  if ($requirements['ultimate_cron']['severity'] != REQUIREMENT_OK) {
    drupal_set_message($requirements['ultimate_cron']['value'], 'error');
    drupal_set_message($requirements['ultimate_cron']['description'], 'error');
  }

  // Get hooks and their data
  $hooks = ultimate_cron_get_hooks();
  $data = _ultimate_cron_preload_cron_data();

  $modules = array();
  foreach ($hooks as $function => $hook) {
    if (!$module || $module == $hook['module']) {
      $hook['settings'] = $data[$function]['settings'] + $hook['settings'];
      $hook['background_process'] = $data[$function]['background_process'];
      $hook['log'] = ultimate_cron_get_log($function);
      $modules[$hook['module']][$function] = $hook;
    }
  }

  $headers = array('', t('Module'), t('Function'), t('Rules'), t('Start'), t('Duration'), t('Status'), array('colspan' => 4, 'data' => ''));
  $output = '';
  $rows = array();

  $overview = array();
  $overview['running'] = 0;
  $overview['errors'] = 0;

  foreach ($modules as $module => $hooks) {
    foreach ($hooks as $function => $hook) {
      // Setup settings
      $conf = $hook['settings'];
      $rules = $hook['settings']['rules'];

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
        if ($process->status == BACKGROUND_PROCESS_STATUS_RUNNING) {
          $log['start'] = $process->start;
        }
        else {
          $log['start'] = NULL;
        }
        $log['end'] = microtime(TRUE);
      }

      if (isset($log['status']) && !$log['status']) {
        $overview['errors']++;
      }
      $link_configure = '';
      if (!empty($hook['configure'])) {
        $link_configure = _ultimate_cron_l('Settings', $hook['configure']);
      }

      $link_unlock = '';
      if ($process) {
        $link_unlock = _ultimate_cron_l('Unlock', 'background-process/unlock/' . $process->handle);
      }

      $link_settings = _ultimate_cron_l('Schedule', 'admin/config/system/cron/settings/' . $function);
      $link_execute = _ultimate_cron_l('Run', 'admin/ultimate-cron/service/start/' . $function);
      $link_log = _ultimate_cron_l('Log', 'admin/reports/cron/' . $function);

      $enable = !empty($conf) && empty($conf['enabled']);
      $link_toggle = _ultimate_cron_l($enable ? 'Enable' : 'Disable', 'admin/ultimate-cron/service/' . ($enable ? 'enable' : 'disable') . '/' . $function);

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
      $data[7]['title'] = t('Schedule');
      $data[8]['data'] = $link_configure;
      $data[8]['title'] = $link_configure ? t('Settings') : '';
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

  if ($overview['running']) {
    drupal_set_message(t('@jobs jobs are currently running', array('@jobs' => $overview['running'])));
  }
  if ($overview['errors']) {
    drupal_set_message(t('@jobs jobs failed their last run', array('@jobs' => $overview['errors'])), 'error');
  }

  return $output;
}

/**
 * Function log page.
 */
function ultimate_cron_function_log_page($function) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[(string)$function])) {
    drupal_not_found();
    exit;
  }

  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');

  $header = array(
    array('data' => t('Start'), 'field' => 'start', 'sort' => 'DESC'),
    array('data' => t('End'),   'field' => 'end'),
    t('Duration'),
    t('Status'),
    t('Message'),
  );

  drupal_set_title(check_plain($function));

  $query = db_select('ultimate_cron_log', 'l');
  $query = $query->condition('l.function', $function)
    ->extend('PagerDefault')
    ->limit(10)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->fields('l', array('lid', 'function', 'start', 'end', 'status', 'msg'))
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
  if (!isset($hooks[(string)$function])) {
    drupal_not_found();
    exit;
  }

  if ($modules = _ultimate_cron_incompatible_modules()) {
    drupal_set_message(t('%function could not start (incompatible module installed)', array('%function' => $function)), 'error');
    drupal_set_message(t('%modules is installed on the system, but is incompatible with Ultimate Cron.<br/>Please disable Parallel Cron.<br/>You might want to !url settings first.', array('%modules' => join(', ', $modules), '!url' => l(t('import'), 'admin/settings/cron/import'))), 'error');
    drupal_goto();
  }

  if (!empty($hooks['unsafe'])) {
    drupal_set_message(t('%function could not start (unsafe)', array('%function' => $function)), 'error');
    drupal_goto();
  }

  // When run manually don't double check the rules
  $hooks[$function]['skip_catch_up'] = TRUE;

  ultimate_cron_load_hook_data($hooks[$function]);
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
  $conf = ultimate_cron_get_settings($function);
  $conf['enabled'] = $enabled;
  ultimate_cron_set_settings($function, $conf);
  drupal_goto();
}

/**
 * Import form.
 */
function ultimate_cron_import_form() {
  $form = array();
  $options = array();

  if ($options) {
    $form['import']['module'] = array(
      '#type' => 'select',
      '#options' => $options,
      '#title' => t('Module'),
      '#description' => t('Module to import settings from'),
    );
    $form['import']['submit'] = array(
      '#type' => 'submit',
      '#submit' => array('ultimate_cron_import_form_submit'),
      '#value' => t('Import'),
    );
  }
  return $form;
}

/**
 * Submit handler for import.
 */
function ultimate_cron_import_form_submit($form, &$form_state) {
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
      'query' => drupal_get_destination(),
      'html' => TRUE,
    )
  );
}
