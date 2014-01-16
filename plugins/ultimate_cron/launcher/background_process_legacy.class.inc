<?php
/**
 * @file
 * Default cron job launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronBackgroundProcessLegacyLauncher extends UltimateCronLauncher {
  static private $currentThread = NULL;

  public function build_operations_alter(&$allowed_operations) {
    if (!empty($allowed_operations['run'])) {
      $allowed_operations['run']['attributes'] = array('class' => array('use-ajax'));
    }
  }
  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'service_group' => variable_get('background_process_default_service_group', ''),
    );
  }

  public function isValid($job = NULL) {
    // Intermistic way of determining version of Background Process.
    // Background Process 1.x has a dependency on the Progress module.
    if (module_exists('background_process')) {
      $info = system_get_info('module', 'background_process');
      if (!empty($info['dependencies']) && in_array('progress', $info['dependencies'])) {
        return parent::isValid($job);
      }
    }
    return FALSE;
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $methods = module_invoke_all('service_group');
    $options = $this->getServiceGroups();
    foreach ($options as $key => &$value) {
      $value = (empty($value['description']) ? $key : $value['description']) . ' (' . join(',', $value['hosts']) . ') : ' . $methods['methods'][$value['method']];
    }

    $elements['service_group'] = array(
      '#title' => t("Service group"),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $values['service_group'],
      '#description' => t('Service group to use for this job.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }

  /**
   * Get service hosts defined in the system.
   */
  protected function getServiceGroups() {
    if (function_exists('background_process_get_service_groups')) {
      return background_process_get_service_groups();
    }

    // Fallback for setups that havent upgraded Background Process.
    // We have this to avoid upgrade dependencies or majer version bump.
    $service_groups = variable_get('background_process_service_groups', array());
    $service_groups += array(
      'default' => array(
        'hosts' => array(variable_get('background_process_default_service_host', 'default')),
      ),
    );
    foreach ($service_groups as &$service_group) {
      $service_group += array(
        'method' => 'background_process_service_group_random'
      );
    }
    return $service_groups;
  }

  /**
   * Settings form validator.
   */
  public function settingsFormValidate($form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];
  }

  /**
   * Background Process doesn't internally provide a unique id
   * for the running process, so we'll have to add that ourselves.
   */
 function lock($job) {
    $lock_id = 'uc-' . $job->name . '-' . urlencode(uniqid('', TRUE));
    $process = new BackgroundProcess($lock_id);
    if ($process->lock()) {
      return $lock_id;
    }
    return FALSE;
  }

  /**
   * Unlock background process.
   */
 function unlock($lock_id) {
    return background_process_unlock($lock_id);
  }

  /**
   * Because Background Process doesn't support a unique id per
   * process, we'll have to match against the prefix, which is
   * the job name.
   */
  function isLocked($job) {
    $lock_id_prefix = 'uc-' . $job->name . '-';
    $lock_id = db_select('background_process', 'bp')
      ->fields('bp', array('handle'))
      ->condition('bp.handle', db_like($lock_id_prefix) . '%', 'LIKE')
      ->range(0, 1)
      ->execute()
      ->fetchField();
    return $lock_id;
  }

  /**
   * Background Process launch.
   */
  function launch($job) {
    $settings = $job->getSettings();

    $lock_id = $job->lock();

    $process = new BackgroundProcess($lock_id);
    $this->exec_status = $this->status = BACKGROUND_PROCESS_STATUS_LOCKED;

    // Always run cron job as anonymous user
    $process->uid = 0;
    $process->service_group = $settings['launcher']['background_process_legacy']['service_group'];

    $service_host = $process->determineServiceHost();

    if (self::$currentThread) {
      $init_message = t('Launched at service host @name', array(
        '@name' => $service_host,
      ));
    }
    else {
      global $user;
      $init_message = t('Launched at service host @name by @username', array(
        '@name' => $service_host,
        '@username' => $user->name,
      ));
    }

    $log = $job->startLog($lock_id, $init_message);

    // We want to finish the log in the sub-request.
    $log->unCatchMessages();

    $process->execute('ultimate_cron_background_process_legacy_callback', array($job->name, $log->log_entry->lid));
  }

  /**
   * Launch manager.
   */
  static public function launchJobs($jobs) {
    /*
    $plugin = ultimate_cron_plugin_load('launcher', 'serial');
    $settings = $plugin->getDefaultSettings();

    // Set proper max execution time.
    $max_execution_time = ini_get('max_execution_time');
    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }
    $lock_timeout = max($max_execution_time, $settings['max_execution_time']);

    // If infinite max execution, then we use a day for the lock.
    $lock_timeout = $lock_timeout ? $lock_timeout : 86400;

    // Find a free slot, try for 3 seconds.
    $lock = FALSE;
    $delay = 3 * 1000000;
    $sleep = 25000;

    while ($delay > 0) {
      for ($slot = 1; $slot <= $settings['max_launch_threads']; $slot++) {
        $lock_name = 'ultimate_cron_serial_launcher_' . $slot;
        if (lock_may_be_available($lock_name)) {
          if ($lock = lock_acquire($lock_name, $lock_timeout)) {
            break 2;
          }
        }
        usleep($sleep);
        // After each sleep, increase the value of $sleep until it reaches
        // 500ms, to reduce the potential for a lock stampede.
        $delay = $delay - $sleep;
        $sleep = min(500000, $sleep + 25000, $delay);
      }
    }

    if (!$lock) {
      watchdog('serial_launcher', "No free slots available for starting launch thread", array(), WATCHDOG_WARNING);
      return;
    }
    watchdog('serial_launcher', "Cron thread %slot started", array('%slot' => $slot), WATCHDOG_INFO);
    */

    self::$currentThread = 1;
    foreach ($jobs as $job) {
      $job->launch();
    }
  }
}
