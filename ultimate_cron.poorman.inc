<?php
/**
 * @file
 * Poormans cron functions.
 */

/**
 * Output buffer callback for poormans cron early page flush.
 */
function _ultimate_cron_poormans_page_flush($content) {
  if (!empty($GLOBALS['poorman_active'])) {
    $size = strlen($content);
    header("Content-Length: $size");
    header("Connection: close");
  }
  return $content;
}
if (ultimate_cron_poorman_capable()) {
  ob_start('_ultimate_cron_poormans_page_flush');
}

/**
 * Determines if poormans cron can run on this page.
 */
function ultimate_cron_poorman_capable() {
  static $capable;
  if (isset($capable)) {
    return $capable;
  }

  if (!drupal_is_cli() && $_SERVER['REQUEST_METHOD'] == 'GET') {
    $capable = TRUE;
  }
  else {
    $capable = FALSE;
  }
  return $capable;
}

/**
 * Implements hook_exit().
 */
function ultimate_cron_exit($dest = NULL) {
  // No need for poorman until site is installed.
  if (variable_get('install_task') != 'done') {
    return;
  }

  $settings = ultimate_cron_plugin_load('settings', 'general')->getDefaultSettings();
  if (!$settings['poorman']) {
    return;
  }

  // We're picky about when we run poormans cron, because
  // we may want to flush the output buffer.
  if (ultimate_cron_poorman_capable()) {
    // Poormans cron needs to be the last that runs.
    // Run remaining exit hooks, and shutdown like core does.
    $modules = module_implements('exit');

    do {
      $module = array_shift($modules);
    } while ($modules && ($module !== 'ultimate_cron'));

    foreach ($modules as $module) {
      module_invoke($module, 'exit', $dest);
    }

    // Commit the user session, if needed.
    drupal_session_commit();

    if (variable_get('cache', 0) && ($cache = drupal_page_set_cache())) {
      drupal_serve_page_from_cache($cache);
    }
    else {
      ob_flush();
    }

    _registry_check_code(REGISTRY_WRITE_LOOKUP_CACHE);
    drupal_cache_system_paths();
    module_implements_write_cache();
    ultimate_cron_launch_poorman();
    exit;
  }
}

/**
 * Launch the poormans thread.
 */
function ultimate_cron_launch_poorman() {
  ultimate_cron_poorman_page_flush();

  // If poorman cron is not running, launch the poormans thread.
  if (!UltimateCronLock::isLocked('ultimate_cron_poorman')) {
    if (lock_acquire('ultimate_cron_poorman_launch')) {
      ultimate_cron_poorman_launcher();
      $expire = microtime(TRUE) + 3;
      while (microtime(TRUE) < $expire) {
        if (UltimateCronLock::isLocked('ultimate_cron_poorman')) {
          lock_release('ultimate_cron_poorman_launch');
          return;
        }
        sleep(1);
      }
      lock_release('ultimate_cron_poorman_launch');
    }
  }
  else {
    // Poormans cron is already running. Ignore.
  }
  return;
}

/**
 * Flush the page and end the client request.
 */
function ultimate_cron_poorman_page_flush() {
  ignore_user_abort();

  if (module_exists('devel') && variable_get('devel_query_display')) {
    if (
      !empty($_SERVER['HTTP_USER_AGENT']) &&
      $_SERVER['HTTP_USER_AGENT'] !== 'Ultimate Cron'
    ) {
      return;
    }
  }

  $GLOBALS['poorman_active'] = TRUE;
  while (ob_get_level() > 0) {
    ob_end_flush();
  }
  flush();

  // FastCGI may need another way of letting the client know, that
  // we're done with it.
  if (function_exists('fastcgi_finish_request')) {
    fastcgi_finish_request();
  }
}

/**
 * Page callback for launching launcher threads.
 */
function ultimate_cron_poorman_page($launcher = NULL) {
  if (!isset($_GET['cron_key']) || variable_get('cron_key', 'drupal') != $_GET['cron_key']) {
    watchdog('cron', 'Cron could not run because an invalid key was used.', array(), WATCHDOG_NOTICE);
    drupal_access_denied();
    exit;
  }

  if (variable_get('maintenance_mode', 0)) {
    // We don't do maintenance mode.
    drupal_access_denied();
    exit;
  }

  if (!$launcher) {
    ultimate_cron_launch_launchers();
  }
  else {
    $launcher = ultimate_cron_plugin_load('launcher', $launcher);
    if (!$launcher) {
      // Invalid launcher specified.
      drupal_not_found();
      exit;
    }

    // Say goodbye to the client.
    ultimate_cron_poorman_page_flush();


    ultimate_cron_run_launcher($launcher);
  }
  exit;
}

/**
 * Launch the launchers.
 */
function ultimate_cron_launch_launchers() {
  $settings = ultimate_cron_plugin_load('settings', 'general')->getDefaultSettings();
  if (!$settings['poorman']) {
    return;
  }

  // Say goodbye to the client.
  ultimate_cron_poorman_page_flush();

  $lock_id = UltimateCronLock::lock('ultimate_cron_poorman', 120);
  if (!$lock_id) {
    // Race-condition or stale lock.
    watchdog('ultimate_cron', 'Could not acquire lock for poorman launcher. Race-condition or stale lock', array(), WATCHDOG_WARNING);
    return;
  }

  // Wait until it's our turn (0 seconds at next minute).
  $cron_last = variable_get('cron_last', 0);
  $cron_next = floor(($cron_last + 60) / 60) * 60;
  $time = time();
  if ($time < $cron_next) {
    $sleep = $cron_next - $time;
    // For debugging purposes. We should really just do sleep($sleep)
    sleep($sleep);
    /*
    while ($sleep--) {
      error_log("SLEEPING1: $sleep");
      sleep(1);
    }
    /**/

    // Time to run cron. Unlock and relaunch, so we get a fresh instance.
    UltimateCronLock::unlock($lock_id);
    if (lock_acquire('ultimate_cron_poorman_launch')) {
      ultimate_cron_poorman_launcher();
    }
    else {
      // Race-condition or stale lock.
    }
    return;
  }

  $launchers = array();
  foreach (ultimate_cron_job_load_all() as $job) {
    $launchers[$job->getPlugin('launcher')->name] = TRUE;
  }
  foreach (array_keys($launchers) as $launcher) {
    $plugin = ultimate_cron_plugin_load('launcher', $launcher);
    if (count($launchers) > 1) {
      ultimate_cron_poorman_launcher($plugin->name);
    }
    else {
      // Only one launcher active, let's just use this thread.
      ultimate_cron_run_launcher($plugin);
    }
  }

  // Re-launch
  $cron_last = $time;
  $cron_next = floor(($cron_last + 60) / 60) * 60;
  $time = time();

  // Don't waste precious CPU cycles. Wait again until it's our turn.
  if ($time < $cron_next) {
    $sleep = $cron_next - $time;
    // For debugging purposes. We should really just do sleep($sleep)
    sleep($sleep);
    /*
    while ($sleep--) {
      error_log("SLEEPING2: $sleep");
      sleep(1);
    }
    /**/
  }

  // And we're done! Unlock and relaunch.
  UltimateCronLock::unlock($lock_id);
  if (lock_acquire('ultimate_cron_poorman_launch')) {
    ultimate_cron_poorman_launcher();
  }
  else {
    // Race-condition or stale lock.
  }
}

/**
 * Run the actual jobs for the specified launcher.
 */
function ultimate_cron_run_launcher($launcher) {
  // We're ready to launch the jobs for the specified launcher.
  _ultimate_cron_variable_save('cron_last', time());

  // Find jobs belonging to this launcher.
  $jobs = array();
  foreach (ultimate_cron_job_load_all() as $name => $job) {
    if ($job->getPlugin('launcher')->name == $launcher->name) {
      // Load the latest log entry, as we want to sort on last start time.
      $job->log_entry = $job->loadLatestLogEntry();
      $jobs[$name] = $job;
    }
  }

  // Sort the jobs and launch them.
  // The reason we want to sort, is so that a broken job cannot block others.
  usort($jobs, '_ultimate_cron_sort_jobs_by_start_time');
  $launcher->launchJobs($jobs);
}

/**
 * Send the actual HTTP request for launching threads.
 */
function ultimate_cron_poorman_launcher($launcher = '') {
  $url = url('admin/config/system/cron/poorman/' . $launcher, array(
    'query' => array('cron_key' => variable_get('cron_key', 'drupal')),
    'absolute' => TRUE,
  ));
  $options = array(
    'headers' => array(
      'User-Agent' => 'Ultimate Cron',
    ),
  );

  $response = drupal_http_request($url, $options);
  if (!empty($response->error)) {
    watchdog('ultimate_cron', 'Could not launch poorman launcher at @url. Error: @error', array(
      '@url' => $url,
      '@error' => $response->error,
    ), WATCHDOG_ERROR);
  }
}
