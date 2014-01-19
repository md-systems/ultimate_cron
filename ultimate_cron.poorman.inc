<?php

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

  // We're picky about when we run poormans cron, because
  // we may want to flush the output buffer.
  if (ultimate_cron_poorman_capable()) {
    // Register our own Poormans Cron handler.
    $callbacks = &drupal_register_shutdown_function();
    array_unshift($callbacks, array('callback' => 'ultimate_cron_poorman', 'arguments' => array()));
    // Reset internal array pointer just in case ...
    reset($callbacks);
  }
}

/**
 * Page callback for running cron jobs.
 */
function ultimate_cron_poorman_page() {
  error_log("POORMAN PAGE");
  if (!isset($_GET['cron_key']) || variable_get('cron_key', 'drupal') != $_GET['cron_key']) {
    watchdog('cron', 'Cron could not run because an invalid key was used.', array(), WATCHDOG_NOTICE);
    drupal_access_denied();
  }
  return 'POORMANS CRON';
}

/**
 * Launch poormans cron if applicable (shutdown handler).
 */
function ultimate_cron_poorman() {
  return;
  ultimate_cron_run_poorman();
  return;

  error_log(__FUNCTION__);
  $settings = ultimate_cron_plugin_load('settings', 'general')->getDefaultSettings();
  $cron_last = variable_get('cron_last', 0);
  error_log("CHECKING");
  $time = time();
  error_log("last: " . format_date($cron_last, 'custom', 'Y-m-d H:i:s'));
  error_log("time: " . format_date($time, 'custom', 'Y-m-d H:i:s'));
  error_log("intv: " . $settings['poorman_interval']);

  if ($settings['poorman_interval'] && ($cron_last < time() - $settings['poorman_interval'])) {
    error_log("LAUNCHING");
    // Thundering herd protection should be done in the launcher.

    // ultimate_cron_poorman_run();
  }
  if ($settings['poorman_interval']) {
  }

  // ultimate_cron_poorman_run();
}

/**
 * Run scheduled jobs.
 */
function ultimate_cron_run_poorman() {
  error_log(__FUNCTION__);
  return;
  if (!variable_get('maintenance_mode', 0)) {
    $schedule = array();
    foreach (ultimate_cron_job_load_all() as $name => $job) {
      $class = get_class($job->getPlugin('launcher'));
      $schedule[$class][$name] = $job;
    }

    _ultimate_cron_variable_save('cron_last', time());

    foreach ($schedule as $class => $jobs) {
      $class::launchPoorman($class, $jobs);
    }
  }
}


function ultimate_cron_poorman_page_flush() {
  error_log(__FUNCTION__);
  if ($settings['poorman_page_flush']) {
    ignore_user_abort();
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
}

function ultimate_cron_trigger_poorman() {
  error_log(__FUNCTION__);
  $url = url('admin/config/system/cron/poorman', array(
    'query' => array('cron_key' => variable_get('cron_key', 'drupal')),
    'absolute' => TRUE,
  ));
  error_log("SLEEPING: $url");
  // drupal_http_request($url);
}

function ultimate_cron_poorman_run() {
  if ($settings['poorman_page_flush']) {
    ignore_user_abort();
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

  ultimate_cron_run_poorman();  
}
