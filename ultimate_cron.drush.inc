<?php
/**
 * @file
 * Drush commands for Ultimate Cron!
 */

/**
 * Implements hook_drush_command().
 */
function ultimate_cron_drush_command() {
  $items = array();

  $items['cron-list'] = array(
    'description' => "List cron jobs.",
    'arguments' => array(
      'type' => 'The type of jobs to list (all, running, enabled, disabled)',
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
    'options' => array(
      'cli' => "Don't spawn a background process through http",
      'check-rule' => "Check rule to determine if job should run",
      'logfile' => "File to log to when spawning cli processes"
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
  $jobs = array();

  $modules = array();
  foreach ($hooks as $name => $hook) {
    if (!$module || $module == $hook['module']) {
      $modules[$hook['module']][$name] = $hook;
    }
  }

  foreach ($hooks as $name => &$hook) {
    if ($module && !in_array($hook['module'], $module)) {
      continue;
    }

    $hook['log'] = ultimate_cron_get_last_log_entry($name);

    switch ($type) {
      case 'enabled':
        if (!empty($hook['enabled'])) {
          $jobs[] = $hook;
        }
        break;

      case 'disabled':
        if (empty($hook['enabled'])) {
          $jobs[] = $hook;
        }
        break;

      case 'running':
        if ($hook['background_process']) {
          $jobs[] = $hook;
        }
        break;

      case 'failed':
        if (isset($hook['log']['status']) && $hook['log']['status'] <= WATCHDOG_ERROR) {
          $jobs[] = $hook;
        }
        break;

      case 'all':
      default:
        $jobs[] = $hook;
    }
  }

  $table = array();
  $table[] = array('', dt('Name'), dt('Module'), dt('Callback'), dt('Rules'), dt('Start'), dt('Duration'));
  foreach ($jobs as $hook) {
    $legend = '';
    $progress = '';
    if (!empty($hook['background_process'])) {
      $legend .= 'R';
      $hook['log']['start_time'] = $hook['background_process']->getStartTime();
      $hook['log']['end_time'] = microtime(TRUE);
      $progress = $hook['background_process']->getProgress();
      $progress = $progress ? sprintf(" (%02d%%)", $progress * 100) : '';
    }
    if (empty($hook['enabled'])) $legend .= 'D';

    $start = isset($hook['log']['start_time']) ? format_date((int)$hook['log']['start_time'], 'custom', 'Y-m-d H:i:s') : dt('N/A');
    $end = isset($hook['log']['end_time']) ? gmdate('H:i:s', (int)($hook['log']['end_time'] - $hook['log']['start_time'])) : dt('N/A');
    $table[] = array($legend, $hook['name'], $hook['module'], $hook['callback'], implode("\n", $hook['rules']), $start, $end . $progress);
  }
  drush_print_table($table);
}

/**
 * Run cron job(s)
 */
function drush_ultimate_cron_cron_run($name = NULL) {
  $cli = drush_get_option('cli');
  $check_rule = drush_get_option('check-rule');
  $logfile = drush_get_option('logfile');
  $logfile = is_string($logfile) ? $logfile : '/dev/null';

  // Get global options
  $options = drush_get_context('cli');
  $cmd_options = '';

  // Determine new parameter string for sub-requests
  $passthru = array('root', 'php', 'uri', 'simulate');
  foreach ($options as $key => $option) {
    if (in_array($key, $passthru)) {
      $cmd_options .= ' --' . $key . '=' . escapeshellarg($option);
    }
  }

  if ($name == 'all') {
    if ($cli) {
      $hooks = ultimate_cron_get_hooks();
      $schedule = ultimate_cron_get_schedule($hooks);
      foreach ($schedule as $name => $hook) {
        if (!empty($options['simulate'])) {
          // Dry-run ...
          drush_print(dt('[!name]: Simulated launch @ !ts', array('!ts' => date('Y-m-d H:i:s'), '!name' => $name)));
          continue;
        }

        drush_print(dt('[!name]: Launching @ !ts', array('!ts' => date('Y-m-d H:i:s'), '!name' => $name)));
        $cmd = $_SERVER['SCRIPT_FILENAME'] . " cron-run $name --cli " . ($check_rule ? '--check-rule' : '');
        exec("$cmd >> " . escapeshellarg($logfile) . " 2>&1 &");
      }
      drush_print(dt('[!ts] Launced !jobs jobs', array('!ts' => date('Y-m-d H:i:s'), '!jobs' => count($schedule))));

      // Update drupals cron timestamp, but don't clear the cache for all variables!
      if (empty($options['simulate'])) {
        _ultimate_cron_variable_set('cron_last', time());
      }
      return;
    }
    else {
      if (empty($options['simulate'])) {
        ultimate_cron_cron_run(TRUE);
      }
      $messages = drupal_get_messages();
      foreach ($messages['status'] as $message) {
        drush_print(strip_tags($message));
      }
      return;
    }
  }

  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$name])) {
    return drush_set_error(dt('[!name]: not found', array('!name' => $name)));
  }

  $hook = $hooks[$name];

  // When run manually don't double check the rules
  if (drush_get_option('check-rule')) {
    if (!ultimate_cron_hook_should_run($hook)) {
      drush_print(dt("[!name]: not sceduled to run at this time", array('!name' => $name)));
      return;
    }
  }


  if ($cli) {
    $start = microtime(TRUE);
    $result = ultimate_cron_run_hook_cli($hook, TRUE);
  }
  else {
    $result = ultimate_cron_run_hook($hook, TRUE);
  }


  if ($result === FALSE) {
    return drush_set_error(dt('[!name]: could not start (already running?)', array('!name' => $name)));
  }

  if ($cli) {
    $log = ultimate_cron_get_log($name);
    if ($log['start'] >= $start && !empty($log['msg'])) {
      drush_print("[$name]: " . $log['msg']);
    }
    #drush_print(dt('!name finished', array('!name' => $name)));
    return $result ? NULL : drush_set_error(dt('[!name]: could not start (service unavailable)', array('!name' => $name)));
  }

  if ($result === NULL) {
    return drush_set_error(dt('[!name]: could not start (service unavailable)', array('!name' => $name)));
  }
  else {
    drush_print(dt('!name started', array('!name' => $name)));
  }
}

/**
 * Enable a cron job
 */
function drush_ultimate_cron_cron_enable($name) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$name])) {
    return drush_set_error(dt('"!name" not found', array('!name' => $name)));
  }
  $settings = ultimate_cron_load($name);
  $settings->settings = $settings->settings ? $settings->settings : array();
  $settings->settings['enabled'] = TRUE;
  ultimate_cron_save($settings);
  drush_print(dt('!name enabled', array('!name' => $name)));
}

/**
 * Disable a cron job
 */
function drush_ultimate_cron_cron_disable($name) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$name])) {
    return drush_set_error(dt('"!name" not found', array('!name' => $name)));
  }
  $settings = ultimate_cron_load($name);
  $settings->settings = $settings->settings ? $settings->settings : array();
  $settings->settings['enabled'] = FALSE;
  ultimate_cron_save($settings);
  drush_print(dt('!name disabled', array('!name' => $name)));
}

/**
 * Unlock a cron job
 */
function drush_ultimate_cron_cron_unlock($name) {
  $hooks = ultimate_cron_get_hooks();
  if (!isset($hooks[$name])) {
    return drush_set_error(dt('"!name" not found', array('!name' => $name)));
  }
  $hook = $hooks[$name];

  if (!$hook['background_process']) {
    drush_print(dt('Cron job !name is not running', array('!name' => $name)));
    return;
  }

  // Unlock the process
  if ($hook['background_process']->unlock()) {
    drush_print(dt('Cron job !name unlocked (process handle: !handle)', array('!handle' => $hook['background_process']->getHandle(), '!name' => $name)));
  }
  else {
    drush_set_error(dt('Could not unlock cron job !name (process handle: !handle)', array('!handle' => $hook['background_process']->getHandle(), '!name' => $name)));
  }
}

