<?php
/**
 * @file
 */

/**
 * Menu callback: runs cron and returns to status-report page.
 */
function ultimate_cron_run_cron() {
  // Run the cron and return
  ultimate_cron_cron_run(TRUE);
  drupal_goto('admin/reports/status');
}

/**
 * Settings form.
 */
function ultimate_cron_settings_form() {
  $form = array();
  $advanced_help_enabled = module_exists('advanced_help');

  $form['general'] = array(
    '#title' => t('General'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $service_groups = $options = background_process_get_service_groups();
  foreach ($options as $key => &$value) {
    $value = (empty($value['description']) ? $key : $value['description']) . ' (' . join(',', $value['hosts']) . ')';
  }
  $form['general']['ultimate_cron_service_group'] = array(
    '#type' => 'select',
    '#title' => t('Service group'),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'service_group',
                              'type' => 'icon')
                      ) : '') . t('Service group to use for all jobs. See Background Process !url for managing service groups.', array('!url' => l(t('settings'), 'admin/config/system/background-process'))),
    '#options' => $options,
    '#default_value' => variable_get('ultimate_cron_service_group', ULTIMATE_CRON_SERVICE_GROUP),
  );

  $form['rules'] = array(
    '#title' => t('Rules'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['rules']['ultimate_cron_rule'] = array(
    '#title' => t("Default rule"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_rule', ULTIMATE_CRON_RULE),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'rules',
                              'type' => 'icon')
                      ) : '') . t('Enter the default fallback rule'),
  );
  $form['rules']['ultimate_cron_catch_up'] = array(
    '#title' => t('Default catch up'),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_catch_up', ULTIMATE_CRON_CATCH_UP),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'catch_up',
                              'type' => 'icon')
                      ) : '') . t('Time in seconds to catch up, if a job could not be run within its time frame. (blank = ' . variable_get('ultimate_cron_catch_up', ULTIMATE_CRON_CATCH_UP) . ')'),
  );

  $form['cleanup'] = array(
    '#title' => t('Clean up'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['cleanup']['ultimate_cron_cleanup_log'] = array(
    '#title' => t("Clean up logs older than X seconds"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_cleanup_log', ULTIMATE_CRON_CLEANUP_LOG),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'cleanup_log',
                              'type' => 'icon')
                      ) : '') . t('Enter maximum age, in seconds, for log entries'),
  );

  $form['queues'] = array(
    '#title' => t('Queues'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['queues']['ultimate_cron_queue_polling_latency'] = array(
    '#title' => t("Queue polling latency"),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_queue_polling_latency', ULTIMATE_CRON_QUEUE_POLLING_LATENCY),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'polling_latency',
                              'type' => 'icon')
                      ) : '') . t('Queue polling latency in miliseconds. Leave blank to disable continuous processing of queues.'),
  );
  $form['queues']['ultimate_cron_queue_lease_time'] = array(
    '#title' => t('Queue lease time'),
    '#type' => 'textfield',
    '#default_value' => variable_get('ultimate_cron_queue_lease_time', ULTIMATE_CRON_QUEUE_LEASE_TIME),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'queue_lease_time',
                              'type' => 'icon')
                      ) : '') . t('Time in seconds to keep lock on claimed item'),
  );

  $form['poorman'] = array(
    '#title' => t('Poormans cron'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => FALSE,
  );
  $form['poorman']['ultimate_cron_poorman'] = array(
    '#title' => t("Poormans cron"),
    '#type' => 'checkbox',
    '#default_value' => variable_get('ultimate_cron_poorman', ULTIMATE_CRON_POORMAN),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'poorman',
                              'type' => 'icon')
                      ) : '') . t('Keep background process alive, checking for cron every minute.'),
  );

  $form = system_settings_form($form);
  return $form;
}

/**
 * Function settings form.
 */
function ultimate_cron_function_settings_form($form, &$form_state, $name) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[(string)$name])) {
    drupal_not_found();
    exit;
  }

  // Load settings
  $hook = $hooks[$name];
  #var_dump($hook);
  $settings = ultimate_cron_load($name);
  $settings = $settings ? $settings->settings : array();
  $settings += array(
    'enabled' => TRUE,
    'rules' => array(),
    'catch up' => '',
    'service group' => '',
    'queue lease time' => '',
    'queue polling latency' => '',
  );

  // Setup form
  drupal_set_title(check_plain($name));
  $form = array();
  $advanced_help_enabled = module_exists('advanced_help');

   // General settings -----------------------------------
  $form['name'] = array(
    '#type' => 'value',
    '#value' => $name,
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
    '#default_value' => $settings['enabled'],
    '#description' => t('Enable this cron job.'),
  );
  $form['general']['rules'] = array(
    '#title' => t('Rules'),
    '#type' => 'textfield',
    '#default_value' => implode(';', $settings['rules']),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'rules',
                              'type' => 'icon')
                      ) : '') . t('Semi-colon separated list of rules for this job. (blank = ' . implode(';', $hook['rules']) . ')'),
  );
  $form['general']['catch up'] = array(
    '#title' => t('Catch up'),
    '#type' => 'textfield',
    '#default_value' => $settings['catch up'],
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'catch_up',
                              'type' => 'icon')
                      ) : '') . t('Time in seconds to catch up, if a job could not be run within its time frame. (blank = ' . $hook['catch up'] . ')'),
  );
  if (isset($hook['queue lease time'])) {
    $form['general']['queue lease time'] = array(
      '#title' => t('Queue lease time'),
      '#type' => 'textfield',
      '#default_value' => $settings['queue lease time'],
      '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                                'module' => 'ultimate_cron',
                                'topic' => 'queue lease time',
                                'type' => 'icon')
                        ) : '') . t('Time in seconds to keep lock on claimed item. (blank = ' . $hook['queue lease time'] . ')'),
    );
    $form['general']['queue polling latency'] = array(
      '#title' => t("Queue polling latency"),
      '#type' => 'textfield',
      '#default_value' => $settings['queue polling latency'],
      '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                                'module' => 'ultimate_cron',
                                'topic' => 'polling_latency',
                                'type' => 'icon')
                        ) : '') . t('Queue polling latency in miliseconds. (blank = ' . (is_numeric($hook['queue polling latency']) ? $hook['queue polling latency'] : t('disabled')) . ')'),
    );
  }

  $service_groups = $options = background_process_get_service_groups();
  foreach ($options as $key => &$value) {
    $value = (empty($value['description']) ? $key : $value['description']) . ' (' . join(',', $value['hosts']) . ')';
  }
  $options += array(
    NULL => 'Ultimate Cron service group (' . join(',', $service_groups[variable_get('ultimate_cron_service_group', ULTIMATE_CRON_SERVICE_GROUP)]['hosts']) . ')');
  $form['general']['service group'] = array(
    '#type' => 'select',
    '#title' => t('Service group'),
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'service_group',
                              'type' => 'icon')
                      ) : '') . t('Service group to use for this job. See Background Process !url for managing service groups.', array('!url' => l(t('settings'), 'admin/config/system/background-process'))),
    '#options' => $options,
    '#default_value' => $settings['service group'],
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
  $settings =& $form_state['values']['general'];
  $settings['rules'] = trim($settings['rules']);

  // Clean up blank values
  foreach ($settings as $idx => $value) {
    if ($value === '') {
      unset($settings[$idx]);
    }
  }

  if ($settings['rules']) {
    $settings['rules'] = $settings['rules'] ? explode(';', $settings['rules']) : array();
    foreach ($settings['rules'] as &$rule) {
      $rule = trim($rule);
      if (!ultimate_cron_validate_rule($rule)) {
        form_set_error('rules', t('Invalid rule.'));
      }
    }
  }
}

/**
 * Submit handler for function settings.
 */
function ultimate_cron_function_settings_form_submit($form, &$form_state) {
  $settings =& $form_state['values']['general'];
  $obj = new stdClass();
  $obj->name = $form_state['values']['name'];
  $obj->settings = $settings;
  ultimate_cron_save($obj);
  unset($form_state['storage']);
}

/**
 * Page overviewing cron jobs.
 */
function ultimate_cron_view_page($module = NULL) {
  require_once 'CronRule.class.php';
  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');

  module_load_install('ultimate_cron');
  $requirements = ultimate_cron_requirements('runtime');
  if ($requirements['ultimate_cron']['severity'] != REQUIREMENT_OK) {
    drupal_set_message($requirements['ultimate_cron']['value'], 'error');
    drupal_set_message($requirements['ultimate_cron']['description'], 'error');
  }

  // Get hooks and their data
  $hooks = ultimate_cron_get_hooks();

  $modules = array();
  foreach ($hooks as $name => $hook) {
    if (!$module || $module == $hook['module']) {
      $hook['log'] = ultimate_cron_get_last_log_entry($name);
      $modules[$hook['module']][$name] = $hook;
    }
  }

  $headers = array('', t('Module'), t('Function'), t('Rules'), t('Start'), t('Duration'), t('Status'), array('colspan' => 3, 'data' => ''), l(t('Run all'), 'admin/reports/status/run-cron', array('query' => drupal_get_destination())));
  $output = '';
  $rows = array();

  $overview = array();
  $overview['running'] = 0;
  $overview['success'] = 0;
  $overview['info'] = 0;
  $overview['warning'] = 0;
  $overview['error'] = 0;

  foreach ($modules as $module => $hooks) {
    foreach ($hooks as $function => $hook) {
      // Setup settings
      $rules = $hook['rules'];
      $cron = new CronRule();
      $parsed_rules = array();
      foreach ($rules as $rule) {
        $cron->rule = $rule;
        $cron->offset = $hook['original weight'];
        $parsed_rules[] = $cron->parseRule();
      }

      // Setup process
      $process = $hook['background_process'];
      $service_host = !$process || !$process->getServiceHost() ? t('N/A') : $process->getServiceHost();

      // Setup log
      $log = $hook['log'];
      if (!$log) {
        $log = array(
          'severity' => -1,
          'start_time' => NULL,
          'end_time' => NULL,
        );
      }
      $severity_type = $log['severity'] < 0 ? 'success' : ($log['severity'] >= WATCHDOG_NOTICE ? 'info' : ($log['severity'] >= WATCHDOG_WARNING ? 'warning' : 'error'));
      $css_status = !empty($process) ? 'running' : $severity_type;

      $short_msg = $log['severity'];
      $msg = !empty($log['msg']) ? $log['msg'] : t('No errors');
      $percentage = '';
      if ($process) {
        $overview['running']++;
        $log['previous_start_time'] = $log['start_time'];
        $log['previous_end_time'] = $log['end_time'];
        $log['start_time'] = $process->getStartTime();
        if ($process->getStatus() == BACKGROUND_PROCESS_STATUS_RUNNING) {
          $log['end_time'] = microtime(TRUE);
          $progress = $process->getProgress();
          $percentage = $progress ? sprintf(" (%02d%%)", $progress * 100) : '';
        }
        else {
          $log['end_time'] = NULL;
        }
      }

      $overview[$severity_type]++;

      $link_configure = '';
      if (!empty($hook['configure'])) {
        $link_configure = _ultimate_cron_l('Settings', $hook['configure']);
      }

      $link_unlock = '';
      if ($process) {
        $link_unlock = _ultimate_cron_l('Unlock', 'background-process/unlock/' . $process->pid);
      }

      $link_settings = _ultimate_cron_l('Schedule', 'admin/config/system/cron/settings/' . $function);
      $link_execute = _ultimate_cron_l('Run', 'admin/ultimate-cron/service/start/' . $function);
      $link_log = _ultimate_cron_l('Log', 'admin/reports/cron/' . $function);

      $enable = empty($hook['enabled']);
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
      $data[3]['title'] = join("\n", $parsed_rules);
      $data[4]['data'] = $log['start_time'] ? format_date((int)$log['start_time'], 'custom', 'Y-m-d H:i:s') : t('Never');
      $data[5]['data'] = $log['end_time'] ? gmdate('H:i:s', (int)($log['end_time'] - $log['start_time'])) . $percentage : ($process ? t('Starting') : t('N/A'));
      $finish = !empty($log['previous_end_time']) ? $log['previous_end_time'] : $log['end_time'];
      $data[5]['title'] = t('Previous run finished @ !timestamp', array(
        '!timestamp' => $finish ? format_date((int)$finish, 'custom', 'Y-m-d H:i:s') : t('N/A')
      ));
      if (!empty($log['previous_start_time'])) {
        $data[4]['title'] = t('Previous run started @ !timestamp', array(
          '!timestamp' => format_date((int)$log['previous_start_time'], 'custom', 'Y-m-d H:i:s'),
        ));
        $data[5]['title'] .= ' - ' . t('Run time: !duration', array(
          '!duration' => gmdate('H:i:s', (int)($log['previous_end_time'] - $log['previous_start_time'])),
        ));
      }
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
      $data[10]['data'] = ($process ? $link_unlock : $link_execute);
      $data[10]['title'] = ($process ? t('Unlock') : t('Run'));

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
  if ($overview['warning']) {
    drupal_set_message(t('@jobs jobs had errors warnings their last run', array('@jobs' => $overview['warning'])), 'warning');
  }
  if ($overview['error']) {
    drupal_set_message(t('@jobs jobs had errors during their last run', array('@jobs' => $overview['error'])), 'error');
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
    array('data' => t('Start'), 'field' => 'start_time', 'sort' => 'DESC'),
    array('data' => t('End'),   'field' => 'end_time'),
    t('Duration'),
    t('Status'),
    t('Message'),
  );

  drupal_set_title(check_plain($function));

  $query = db_select('ultimate_cron_log', 'l');
  $query = $query->condition('l.name', $function)
    ->extend('PagerDefault')
    ->limit(10)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->fields('l', array('lid', 'name', 'start_time', 'end_time', 'severity', 'msg'));
  $logs = $query->execute()->fetchAll(PDO::FETCH_ASSOC);

  $output = '';
  $rows = array();
  foreach ($logs as $log) {
    $severity_type = $log['severity'] < 0 ? 'success' : ($log['severity'] >= WATCHDOG_INFO ? 'info' : ($log['severity'] >= WATCHDOG_WARNING ? 'warning' : 'error'));
    $css_status = !empty($process) ? 'running' : $severity_type;
    $data = array(
      array('class' => array('ultimate-cron-admin-start')),
      array('class' => array('ultimate-cron-admin-end')),
      array('class' => array('ultimate-cron-admin-duration')),
      array('class' => array('ultimate-cron-admin-status ultimate-cron-admin-status-' . $css_status)),
      array('class' => array('ultimate-cron-admin-message')),
    );
    $data[0]['data'] = format_date((int)$log['start_time'], 'custom', 'Y-m-d H:i:s');
    $data[1]['data'] = format_date((int)$log['end_time'], 'custom', 'Y-m-d H:i:s');
    $data[2]['data'] = gmdate('H:i:s', (int)($log['end_time'] - $log['start_time']));
    $data[3]['data'] = '<span>' . $log['severity'] . '</span>';
    $data[4]['data'] = $log['msg'];
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

  // When run manually don't double check the rules
  $hooks[$function]['skip_catch_up'] = TRUE;

  try {
    $process = ultimate_cron_run_hook($hooks[$function]);
    drupal_set_message(t('%function started', array('%function' => $function)));
  }
  catch (Exception $e) {
    drupal_set_message(t('%function could not start (%error)', array('%function' => $function, '%error' => (string) $e)), 'error');
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
