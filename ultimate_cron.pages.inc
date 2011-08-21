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
    '#default_value' => implode(',', $conf['rules']),
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
  $conf['rules'] = $conf['rules'] ? explode(',', $conf['rules']) : array();
  
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
  $hooks = ultimate_cron_get_hooks();
  $modules = array();
  foreach ($hooks as $function => $hook) {
    if (!$module || $module == $hook['module']) {
      $modules[$hook['module']][$function] = $hook;
    }
  }

  $charts_graphs_installed = module_exists('charts_graphs');

  $edit_links = module_invoke_all('cronapi', 'edit');
  
  $headers = array('', t('Module'), t('Function'), t('Rules'), t('Start'), t('Duration'), t('Status'), t('Action'));
  $output = '';
  $rows = array();
  
  foreach ($modules as $module => $hooks) {
    foreach ($hooks as $function => $hook) {
      $process = $hook['background_process'];
      
      $log = ultimate_cron_get_log($function);
      $conf = $hook['configuration'];
      $msg = $log['status'] === NULL ? '' : ($log['status'] ? t('No errors') : strip_tags($log['msg']));
      $msg = $msg ? $msg : t('No information');
      $service_host = empty($process->service_host) ? t('default') : $process->service_host;

      if ($charts_graphs_installed) {
        $link_log = l(
          '<img title="' . t('Log') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/chart_line.png"/>', 
          'admin/config/system/cron/log/' . $function, 
          array(
            'html' => TRUE, 
            'attributes' => array(
              'rel' => 'lightframe',
              'target' => '_blank',
            )
          )
        );
      }

      $link_edit = '';
      $edit_links[$function] = empty($edit_links[$function]) && isset($hook['configure']) ? $hook['configure'] : $edit_links[$function];
      $link_log = '';
      if (!empty($edit_links[$function])) {
        $link_edit = l('<img title="' . t('Edit') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/application_form_edit.png"/>', $edit_links[$function], array('html' => TRUE));
      }

      $inprogress = $process ? TRUE : FALSE;
      
      $link_settings = l('<img title="' . t('Settings') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/cog.png"/>', 'admin/config/system/cron/settings/' . $function, array('html' => TRUE));
      $link_execute = l('<img title="' . t('Run') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/application_go.png"/>', 'admin/ultimate-cron/service/start/' . $function, array('html' => TRUE, 'query' => array('destination' => trim(request_uri(), '/'))));
      if ($process) {
        $link_unlock = l('<img title="' . t('Unlock') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/lock_open.png"/>', 'background-process/unlock/' . $process->handle, array('html' => TRUE, 'query' => array('destination' => trim(request_uri(), '/'))));
      }
      $link_inprogress = '<img title="' . t('Running on @host', array('@host' => $service_host)) . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass.png"/>';
      $link_status = '<img title="' . $msg . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/' . ($log['status'] === NULL ? 'information.png' : ($log['status'] ? 'tick.png' : 'error.png')). '"/>';


      if (empty($conf['enabled'])) {
        $link_toggle = l('<img title="' . t('Enable') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/add.png"/>', 'admin/ultimate-cron/service/enable/' . $function, array('html' => TRUE, 'query' => array('destination' => trim(request_uri(), '/'))));
      }
      else {
        $link_toggle = l('<img title="' . t('Disable') . '" src="/' . drupal_get_path('module', 'ultimate_cron') . '/icons/delete.png"/>', 'admin/ultimate-cron/service/disable/' . $function, array('html' => TRUE, 'query' => array('destination' => trim(request_uri(), '/'))));
      }

      if ($inprogress) {
        $progress = progress_get_progress($process->handle);
        $start = $progress->start;
        $end = microtime(TRUE);
      }
      else {
        $start = $log['start'];
        $end = $log['end'];
      }
      
      $service_group = !empty($conf['service_group']) ? $conf['service_group'] : variable_get('background_process_default_service_group', '');
      $service_group_info = t('N/A');
      if ($service_group) {
        $service_groups = variable_get('background_process_service_groups', array());
        if (isset($service_groups[$service_group])) {
          $service_group_info = "$service_group\n+-- " . join("\n+-- ", $service_groups[$service_group]['hosts']);
        }
      }
      
      $data = array(
        'data' => array(
          $link_toggle,
          l(ultimate_cron_module_name($module), 'admin/config/system/cron/overview/' . $module),
          array(
            'data' => $hook['description'],
            'title' => $service_group_info,
          ),
          is_array($conf['rules']) ? array(
            'data' => $conf['rules'][0],
            'title' => join("\n", $conf['rules']),
          ) : t('N/A'),
          $start ? format_date_hires($start, 'custom', 'Y-m-d H:i:s:u') : ($inprogress ? t('Starting') : t('Never')),
          $start ? array(
            'data' => gmdate('H:i:s:u', $end - $start),
            'title' => t('Finished @ !timestamp', array('!timestamp' => format_date_hires($end, 'custom', 'Y-m-d H:i:s:u'))),
          ) : t('N/A'),
          $inprogress ? $link_inprogress : $link_status,
          $link_log . 
          '&nbsp' . $link_settings .
          ($inprogress ? '&nbsp' . $link_unlock : '&nbsp' . $link_execute) . 
          ($link_edit ? '&nbsp' . $link_edit : ''),
        ),
        'style' => empty($hook['configuration']['enabled']) ? 'color: red' : '',
        'class' => $conf['enabled'] ? '' : 'error',
      );
      $rows[] = $data;
    }
  }
  $output .= theme('table', array(
    'headers' => $headers, 
    'rows' => $rows, 
    'attributes' => array('id' => 'ultimate-cron-view')
  ));

  // drupal_add_js(drupal_get_path('module', 'ultimate_cron') . '/js/jquery.tableSort.js');
  // $script = '$(\'#ultimate-cron-view\').sortTable({ onCol: 2, keepRelations: true });';
  // drupal_add_js($script, 'inline');

  // drupal_add_js(drupal_get_path('module', 'ultimate_cron') . '/js/jquery.tablesorter.min.js');
  // $script = '$(document).ready(function() { $(\'#ultimate-cron-view\').tablesorter({ headers: { 0: { sorter: false }, 6: { sorter: false }, 7: { sorter: false } } }); });';
  // drupal_add_js($script, 'inline');

  return $output;
}

function format_date_hires($timestamp, $type, $format, $timezone = NULL, $langcode = NULL) {
  return date($format, $timestamp);
  $format_date = format_date($timestamp, $type, $format, $timezone, $langcode);
  $ms = sprintf('%03d', ($timestamp - floor($timestamp)) * 1000);
  $format_date = str_replace('u', $ms, $format_date);
  return $format_date;
}

function ultimate_cron_history($function) {
  $canvas = charts_graphs_get_graph('open-flash');

  $logs = ultimate_cron_get_logs($function);
  
  $hooks = ultimate_cron_get_hooks();
  
  $canvas->title = $hooks[$function]['description'];
  $canvas->type = "bar_glass"; // a chart type supported by the charting engine. See further in the doc for the list.
  $canvas->y_legend = t('Seconds');
  #$canvas->colour = '#D54C78';
  $canvas->theme = 'keynote';
  $key = t('Execution Time');
  $canvas->series = array($key => array());
  $canvas->x_labels = array();
  foreach (array_reverse($logs) as $log) {
    $canvas->series[$key][] =  $log['end'] - $log['start'];
    $canvas->x_labels[] = format_date($log['start'], 'custom', 'H:i:s');
  }

  $out = $canvas->get_chart();
  return $out;
}
