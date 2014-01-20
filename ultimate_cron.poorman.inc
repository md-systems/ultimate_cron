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
    ultimate_cron_run_poorman();
    exit;
  }
}

/**
 * Launch poormans cron if applicable (shutdown handler).
 */
function ultimate_cron_run_poorman() {
  error_log(__FUNCTION__);
  if (!variable_get('maintenance_mode', 0)) {
    $schedule = array();
    foreach (ultimate_cron_job_load_all() as $name => $job) {
      $job->log_entry = $job->loadLatestLog()->log_entry;
      $plugin = $job->getPlugin('launcher');
      $schedule[$plugin->name]['plugin'] = $plugin;
      $schedule[$plugin->name]['jobs'][$name] = $job;
    }

    foreach ($schedule as $plugins) {
      usort($plugins['jobs'], '_ultimate_cron_sort_jobs_by_start_time');
      $plugins['plugin']->launchPoorman($plugins['jobs']);
    }
  }
}


function ultimate_cron_poorman_page_flush() {
  error_log(__FUNCTION__);
  $settings = ultimate_cron_plugin_load('settings', 'general')->getDefaultSettings();
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

function ultimate_cron_poorman_page() {
  error_log(__FUNCTION__);
  return 'TEST';
}

function ultimate_cron_trigger_poorman() {
  error_log(__FUNCTION__);
  $url = url('admin/config/system/cron/poorman', array(
    'query' => array('cron_key' => variable_get('cron_key', 'drupal')),
    'absolute' => TRUE,
  ));
  error_log("SLEEPING: $url");
  drupal_http_request($url);
}
