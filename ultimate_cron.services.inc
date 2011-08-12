<?php
/**
 * @file
 */

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
  
  $handle = ultimate_cron_execute($function, $hooks[$function]);

  if ($handle === FALSE) {
    drupal_set_message(t('%function could not start (already running?)', array('%function' => $function)), 'error');
  }
  elseif ($handle === NULL) {
    drupal_set_message(t('%function could not start (service unavailable)', array('%function' => $function)), 'error');
  }
  else {
    drupal_set_message(t('%function started', array('%function' => $function)));
    // return t('Handle: %handle', array('%handle' => $handle));
  }
  // return '';
  drupal_goto();
}

function ultimate_cron_service_enable($function, $enabled) {
  $conf = ultimate_cron_get_configuration($function);
  $conf['enabled'] = $enabled;
  ultimate_cron_set_configuration($function, $conf);
  drupal_goto();
}
