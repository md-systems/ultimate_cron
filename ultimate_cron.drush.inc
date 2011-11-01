<?php

/**
 * @file
 *   Drush commands for Ultimate Cron!
 */

/**
 * Implements hook_drush_command().
 */
function ultimate_cron_drush_command() {
  $items = array();

  $items['cron-list'] = array(
    'description' => "List cron jobs.",
    'arguments' => array(
      'type' => 'The type of jobs to list (all, running, enabled, disabled, unsafe)',
    ),
    'options' => array(
      'module' => 'Only show jobs from comma separated list of modules',
    ),
    'examples' => array(
      'drush cron-list running',
    ),
    'aliases' => array('cl'),
  );

  $items['cron-run'] = array(
    'description' => "Run cron job.",
    'arguments' => array(
      'function' => 'Function to run',
    ),
    'examples' => array(
      'drush cron-run node_cron',
    ),
    'aliases' => array('cr'),
  );

  $items['cron-enable'] = array(
    'description' => "Enable cron job.",
    'arguments' => array(
      'function' => 'Function to enable',
    ),
    'examples' => array(
      'drush cron-enable node_cron',
    ),
    'aliases' => array('ce'),
  );

  $items['cron-disable'] = array(
    'description' => "Disable cron job.",
    'arguments' => array(
      'function' => 'Function to disable',
    ),
    'examples' => array(
      'drush cron-disable node_cron',
    ),
    'aliases' => array('cd'),
  );

  $items['cron-unlock'] = array(
    'description' => "Unlock cron job.",
    'arguments' => array(
      'function' => 'Function to unlock',
    ),
    'examples' => array(
      'drush cron-unlock node_cron',
    ),
    'aliases' => array('cu'),
  );

  return $items;
}

/**
 * Implements hook_drush_help().
 */
function ultimate_cron_drush_help($section) {
  switch ($section) {
    case 'drush:cron-list':
      return dt("This command will list cron jobs.");
    case 'drush:cron-run':
      return dt("This command will run a cron job.");
    case 'drush:cron-enable':
      return dt("This command will enable a cron job.");
    case 'drush:cron-disable':
      return dt("This command will disable a cron job.");
    case 'drush:cron-unlock':
      return dt("This command will unlock a cron job.");
  }
}

/**
 * List cron jobs.
 */
function drush_ultimate_cron_cron_list($type = 'all') {
  $module = drush_get_option('module');
  $module = $module ? explode(",", $module) : array();
  // Get hooks and their data
  $hooks = ultimate_cron_get_hooks();
  $data = _ultimate_cron_preload_cron_data();
  $jobs = array();

  $modules = array();
  foreach ($hooks as $function => $hook) {
    if (!$module || $module == $hook['module']) {
      $modules[$hook['module']][$function] = $hook;
    }
  }

  foreach ($hooks as $function => &$hook) {
    if ($module && !in_array($hook['module'], $module)) {
      continue;
    }

    $hook['settings'] = $data[$function]['settings'] + $hook['settings'];
    $hook['background_process'] = $data[$function]['background_process'];
    $hook['log'] = ultimate_cron_get_log($function);

    switch ($type) {
      case 'enabled':
        if (!empty($hook['settings']['enabled'])) {
          $jobs[] = $hook;
        }
        break;

      case 'disabled':
        if (empty($hook['settings']['enabled'])) {
          $jobs[] = $hook;
        }
        break;

      case 'running':
        if (!empty($data[$hook['function']]['background_process'])) {
          $jobs[] = $hook;
        }
        break;

      case 'unsafe':
        if (!empty($hook['unsafe'])) {
          $jobs[] = $hook;
        }
        break;

      case 'failed':
        if (isset($hook['log']['status']) && empty($hook['log']['status'])) {
          $jobs[] = $hook;
        }
        break;

      case 'all':
      default:
        $jobs[] = $hook;
    }
  }

  $table = array();
  $table[] = array('', dt('Module'), dt('Function'), dt('Rules'), dt('Start'), dt('Duration'));
  foreach ($jobs as $hook) {
    $legend = '';
    if (!empty($hook['background_process'])) {
      $legend .= 'R';
      $hook['log']['start'] = $hook['background_process']->start;
      $hook['log']['end'] = microtime(TRUE);
    }
    if (empty($hook['settings']['enabled'])) $legend .= 'D';

    $start = isset($hook['log']['start']) ? format_date((int)$hook['log']['start'], 'custom', 'Y-m-d H:i:s') : dt('N/A');
    $end = isset($hook['log']['end']) ? gmdate('H:i:s', (int)($hook['log']['end'] - $hook['log']['start'])) : dt('N/A');
    $rules = $hook['settings']['rules'];
    $table[] = array($legend, $hook['module'], $hook['function'], implode("\n", $rules), $start, $end);
  }
  drush_print_table($table);
}

/**
 * Run cron job(s)
 */
function drush_ultimate_cron_cron_run($function = NULL) {
  if ($function == 'all') {
    ultimate_cron_cron(TRUE);
    $messages = drupal_get_messages();
    foreach ($messages['status'] as $message) {
      drush_print(strip_tags($message));
    }
    return;
  }

  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$function])) {
    return drush_set_error(dt('"!function" not found', array('!function' => $function)));
  }

  // When run manually don't double check the rules
  $hooks[$function]['skip_catch_up'] = TRUE;

  $handle = ultimate_cron_run_hook($function, $hooks[$function]);

  if ($handle === FALSE) {
    return drush_set_error(dt('!function could not start (already running?)', array('!function' => $function)));
  }
  elseif ($handle === NULL) {
    return drush_set_error(dt('!function could not start (service unavailable)', array('!function' => $function)));
  }
  else {
    drush_print(dt('!function started', array('!function' => $function)));
  }
}

/**
 * Enable a cron job
 */
function drush_ultimate_cron_cron_enable($function) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$function])) {
    return drush_set_error(dt('"!function" not found', array('!function' => $function)));
  }
  $conf = ultimate_cron_get_settings($function);
  $conf['enabled'] = TRUE;
  ultimate_cron_set_settings($function, $conf);
  drush_print(dt('!function enabled', array('!function' => $function)));
}

/**
 * Disable a cron job
 */
function drush_ultimate_cron_cron_disable($function) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$function])) {
    return drush_set_error(dt('"!function" not found', array('!function' => $function)));
  }
  $conf = ultimate_cron_get_settings($function);
  $conf['enabled'] = FALSE;
  ultimate_cron_set_settings($function, $conf);
  drush_print(dt('!function disabled', array('!function' => $function)));
}

/**
 * Unlock a cron job
 */
function drush_ultimate_cron_cron_unlock($function) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$function])) {
    return drush_set_error(dt('"!function" not found', array('!function' => $function)));
  }

  $handle = 'ultimate_cron:' . $function;
  if ($process = background_process_get_process($handle)) {
    // Unlock the process
    if (background_process_remove_process($process->handle, $process->start)) {
      drush_print(dt('Process for !function unlocked (process handle: !handle)', array('!handle' => $handle, '!function' => $function)));
      module_invoke_all('background_process_shutdown', $process, FALSE, t('Manually unlocked'));
    }
  }
  else {
    drush_set_error(dt('Process for !function not found (process handle: !handle)', array('!handle' => $handle, '!function' => $function)));
  }
}

