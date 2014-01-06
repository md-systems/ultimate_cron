<?php
/**
 * @file
 */

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
  $service_groups = background_process_service_group_load_all();
  $options = array();
  foreach ($service_groups as $name => $service_group) {
    $options[$name] = $service_group->title;
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
                      ) : '') . t('Time in seconds to catch up, if a job could not be run within its time frame.'),
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

  $form_state['values'] = isset($form_state['values']) ? $form_state['values'] : array();
  if (!isset($form_state['values']['launcher_settings'])) {
    $form_state['values']['launcher_settings'] = array();
  }
  $defaults = $launcher->getDefaultSettings();
  $form_state['values']['launcher_settings'] += $defaults;
  $launcher->settingsForm($form, $form_state);
  if (!empty($form['launcher_settings']['no_settings'])) {
    return $form;
  }
  $form['launcher_settings']['#tree'] = TRUE;
  $form['launcher'] = array(
    '#type' => 'value',
    '#value' => $name,
  );
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
  $launcher = ultimate_cron_launcher_load($form_state['values']['launcher']);
  $launcher->settingsFormValidate($form, $form_state);
}

/**
 * Submit handler for launcher settings.
 */
function ultimate_cron_launcher_settings_submit($form, &$form_state) {
  $launcher = ultimate_cron_launcher_load($form_state['values']['launcher']);
  $launcher->settingsFormSubmit($form, $form_state);

  // Since this is a system_settings_form() we need to move the submitted data
  // around a little.
  $form_state['values'][$launcher->key] = $form_state['values']['launcher_settings'];
  unset($form_state['values']['launcher']);
  unset($form_state['values']['launcher_settings']);
}

/**
 * Main launcher settings form.
 */
function ultimate_cron_schedulers_form() {
  $form = array();
  $items = array();
  foreach (ultimate_cron_scheduler_load_all() as $name => $scheduler) {
    $items[] = l($scheduler->title, 'admin/config/system/cron/scheduler/' . $name);
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
 * Job settings form.
 */
function ultimate_cron_function_settings_form($form, &$form_state, $name) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[(string) $name])) {
    drupal_not_found();
    exit;
  }

  // Load settings.
  $hook = $hooks[$name];
  $settings = $hook['settings'];
  $settings += array(
    'enabled' => TRUE,
    'rules' => array(),
    'weight' => '',
    'catch up' => '',
    'service group' => '',
    'queue lease time' => '',
  );

  // Setup form
  drupal_set_title(check_plain($name));
  $form = array();
  $advanced_help_enabled = module_exists('advanced_help');

   // General settings -----------------------------------.
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
                      ) : '') . t('Semi-colon separated list of rules for this job. (blank = ' . implode(';', $hook['defaults']['rules']) . ')'),
  );
  $form['general']['weight'] = array(
    '#title' => t('Weight'),
    '#type' => 'textfield',
    '#default_value' => $settings['weight'],
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'weight',
                              'type' => 'icon')
                      ) : '') . t('Weight. Determines the order of the jobs on the overview page. (blank = ' . $hook['defaults']['weight'] . ')'),
  );
  $form['general']['catch up'] = array(
    '#title' => t('Catch up'),
    '#type' => 'textfield',
    '#default_value' => $settings['catch up'],
    '#description' => ($advanced_help_enabled ? theme('advanced_help_topic', array(
                              'module' => 'ultimate_cron',
                              'topic' => 'catch_up',
                              'type' => 'icon')
                      ) : '') . t('Time in seconds to catch up, if a job could not be run within its time frame. (blank = ' . $hook['defaults']['catch up'] . ')'),
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
                        ) : '') . t('Time in seconds to keep lock on claimed item. (blank = ' . $hook['defaults']['queue lease time'] . ')'),
    );
  }

  $service_groups = background_process_service_group_load_all();
  $options = array();
  foreach ($service_groups as $name => $service_group) {
    $options[$name] = $service_group->title;
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

  if (!empty($settings['rules'])) {
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
function ultimate_cron_jobs_page($status = NULL) {

  require_once 'CronRule.class.php';
  drupal_add_css(drupal_get_path('module', 'ultimate_cron') . '/css/ultimate_cron.admin.css');
  // drupal_add_js(drupal_get_path('module', 'ultimate_cron') . '/js/jquery.tablesorter.js');
  // drupal_add_js(drupal_get_path('module', 'ultimate_cron') . '/js/ultimate_cron.js');

  module_load_install('ultimate_cron');
  $requirements = ultimate_cron_requirements('runtime');
  if ($requirements['ultimate_cron']['severity'] != REQUIREMENT_OK) {
    drupal_set_message($requirements['ultimate_cron']['value'], 'error');
    drupal_set_message($requirements['ultimate_cron']['description'], 'error');
  }

  // Get jobs and their data.
  $jobs = ultimate_cron_get_jobs();

  $output = '';

  $headers = array(t('Module'), t('Description'), t('Rules'), t('Start'), t('Duration'), t('Status'), t('Operations'));
  $rows = array(array(), array());

  $overview = array();
  $overview['running'] = 0;
  $overview['success'] = 0;
  $overview['info'] = 0;
  $overview['warning'] = 0;
  $overview['error'] = 0;

  foreach ($jobs as $name => $job) {
    $log = ultimate_cron_get_last_log_entry($name);
    $log += array(
      'msg' => '',
      'severity' => -1,
      'start_time' => NULL,
      'end_time' => NULL,
    );

    // Setup process.
    $process = BackgroundProcess::loadByHandle('uc:' . $name);
    $service_host = $process && $process->getServiceHost() ? $process->getServiceHost() : t('N/A');

    $severity_type = $log['severity'] < 0 ? 'success' : ($log['severity'] >= WATCHDOG_NOTICE ? 'info' : ($log['severity'] >= WATCHDOG_WARNING ? 'warning' : 'error'));
    $css_status = !empty($process) ? 'running' : $severity_type;
    $css_status = $log['msg'] && ($css_status == 'success') ? 'info' : $css_status;

//    'style' => ($status && $status != 'all' && ($css_status != $status) ? 'display: none' : 
    if ($status && $status != 'all' && ($css_status != $status)) {
      continue;
    }

    // Setup settings.
    $cron = new CronRule();
    $parsed_rules = array();
    foreach ($job['rules'] as $rule) {
      $cron->rule = $rule;
      $cron->offset = 0;
      $parsed_rules[] = $cron->parseRule();
    }

    $operations = array();
    $operations['schedule'] = array(
      'title' => t('Schedule'),
      'href' => 'admin/config/system/cron/settings/' . $name,
      'query' => drupal_get_destination(),
    );
    if (!empty($job['configure'])) {
      $operations['configure'] = array(
        'title' => t('Configure'),
        'href' => $job['configure'],
        'query' => drupal_get_destination(),
      );
    }
    if ($process) {
      $operations['unlock'] = array(
        'title' => t('Unlock'),
        'href' => 'admin/background-process/unlock/' . $process->pid,
        'query' => drupal_get_destination(),
      );
    }
    else {
      $operations['run'] = array(
        'title' => t('Run'),
        'href' => 'admin/ultimate-cron/service/enable/' . $name,
        'query' => drupal_get_destination(),
      );
    }
    if (empty($job['enabled'])) {
      $operations['enable'] = array(
        'title' => t('Enable'),
        'href' => 'admin/ultimate-cron/service/enable',
        'query' => drupal_get_destination(),
      );
    }
    else {
      $operations['disable'] = array(
        'title' => t('Disable'),
        'href' => 'admin/ultimate-cron/service/disable',
        'query' => drupal_get_destination(),
      );
    }

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
        $percentage = $progress >= 0 ? sprintf(" (%3d%%)", $progress * 100) : '';
      }
      else {
        $log['end_time'] = NULL;
      }
    }

    $overview[$severity_type]++;

    $data = array(
      array('class' => 'ultimate-cron-admin-module'),
      array('class' => 'ultimate-cron-admin-function'),
      array('class' => 'ultimate-cron-admin-rules'),
      array('class' => 'ultimate-cron-admin-start'),
      array('class' => 'ultimate-cron-admin-end'),
      array('class' => 'ultimate-cron-admin-status ultimate-cron-admin-status-' . $css_status),
      theme('links__ctools_dropbutton', array(
        'links' => $operations,
        'attributes' => array('class' => array('links', 'inline'))
      )),
    );
    $data[0] += array(
      'data' => ultimate_cron_module_name($job['module']),
      'title' => $job['callback'],
    );
    $data[1] += array(
      'data' => $job['description'],
      'title' => $name,
    );
    $data[2] += array(
      'data' => join("<br/>", $job['rules']),
      'title' => join("\n", $parsed_rules),
    );
    $data[3] += array(
      'data' => $log['start_time'] ? format_date((int) $log['start_time'], 'custom', 'Y-m-d H:i:s') : t('Never'),
    );
    $finish = !empty($log['previous_end_time']) ? $log['previous_end_time'] : $log['end_time'];
    $data[4] += array(
      'data' => $log['end_time'] ? gmdate('H:i:s', (int) ($log['end_time'] - $log['start_time'])) . $percentage : ($process ? t('Starting') : t('N/A')),
      'title' => t('Previous run finished @ !timestamp', array(
        '!timestamp' => $finish ? format_date((int) $finish, 'custom', 'Y-m-d H:i:s') : t('N/A'))
      )
    );

    if ($process) {
      $data[5] += array(
        'data' => '<span>' . t('Running') . '</span>',
        'title' => t('Running on @host', array('@host' => $service_host)),
      );
    }
    else {
      $data[5] += array(
        'data' => '<span>' . $short_msg . '</span>',
        'title' => strip_tags(html_entity_decode($msg, ENT_QUOTES)),
      );
    }

    $rows[(int) $job['enabled']][] = array(
      'class' => array('row-' . $name),
      'data' => $data,
    );
  }

  if ($status != 'all') {
    $msg = 'There are currently no @enabled cron jobs with status %status.';
  }
  else {
    $msg = 'There are currently no @enabled cron jobs.';
  }

  $output .= theme('table', array(
    'header' => $headers,
    'rows' => $rows[1],
    'attributes' => array('id' => 'ctools-export-ui-list-items'),
    'empty' => t($msg, array('@enabled' => t('enabled'), '%status' => $status)),
  ));

  $output .= theme('table', array(
    'header' => $headers,
    'rows' => $rows[0],
    'attributes' => array('id' => 'ctools-export-ui-list-items'),
    'empty' => t($msg, array('@enabled' => t('disabled'), '%status' => $status)),
  ));

  if ($overview['running']) {
    drupal_set_message(format_plural($overview['running'],
                        '@jobs job is currently running',
                        '@jobs jobs are currently running',
                        array('@jobs' => $overview['running'])
                      ));
  }
  if ($overview['warning']) {
    drupal_set_message(format_plural($overview['warning'],
                        '@jobs job had warnings during it\'s last run',
                        '@jobs jobs had warnings during their last run',
                        array('@jobs' => $overview['warning'])
                      ), 'warning');
  }
  if ($overview['error']) {
    drupal_set_message(format_plural($overview['error'],
                        '@jobs job had errors during it\'s last run',
                        '@jobs jobs had errors during their last run',
                        array('@jobs' => $overview['error'])
                      ), 'error');
  }

  return $output;
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
