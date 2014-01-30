<?php
/**
 * @file
 * Background Process 1.x launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronBackgroundProcessLegacyLauncher extends UltimateCronLauncher {
  public $scheduledLaunch = FALSE;

  /**
   * Custom action for plugins.
   */
  public function custom_page($js, $input, $item, $action) {
    switch ($action) {
      case 'end_daemonize':
        $item->sendSignal('end_daemonize', TRUE);
        return;
    }
  }

  /**
   * Use ajax for run, since we're launching in the background.
   */
  public function build_operations_alter($job, &$allowed_operations) {
    if (!empty($allowed_operations['run'])) {
      $allowed_operations['run']['attributes'] = array('class' => array('use-ajax'));
    }
    else {
      $settings = $job->getSettings('launcher');
      if ($settings['daemonize'] && !$job->peekSignal('end_daemonize')) {
        $allowed_operations['end_daemonize'] = array(
          'title' => t('Kill daemon'),
          'href' => 'admin/config/system/cron/jobs/list/' . $job->name . '/custom/' . $this->type . '/' . $this->name . '/end_daemonize',
          'attributes' => array('class' => array('use-ajax')),
        );
      }
    }
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'service_group' => variable_get('background_process_default_service_group', 'default'),
      'max_threads' => 2,
      'daemonize' => FALSE,
      'daemonize_interval' => 10,
      'daemonize_delay' => 1,
    ) + parent::defaultSettings();
  }

  /**
   * Only expose this plugin, if Background Process is 1.x.
   */
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

    if (!$job) {
      $elements['max_threads'] = array(
        '#title' => t("Max threads"),
        '#type' => 'textfield',
        '#default_value' => $values['max_threads'],
        '#description' => t('Maximum number of concurrent cron jobs to run.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
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

    $elements['daemonize'] = array(
      '#title' => t('Daemonize'),
      '#type' => 'checkbox',
      '#default_value' => $values['daemonize'],
      '#description' => t('Relaunch job immediately after it is finished.'),
    );
    $elements['daemonize_interval'] = array(
      '#title' => t('Interval'),
      '#type' => 'textfield',
      '#default_value' => $values['daemonize_interval'],
      '#description' => t('Seconds to run the job in the same thread before relaunching.'),
      '#states' => array(
        'visible' => array(':input[name="settings[' . $this->type . '][' . $this->name . '][daemonize]"]' => array('checked' => TRUE))
      ),
    );
    $elements['daemonize_delay'] = array(
      '#title' => t('Delay'),
      '#type' => 'textfield',
      '#default_value' => $values['daemonize_delay'],
      '#description' => t('Delay in seconds between in job execution.'),
      '#states' => array(
        'visible' => array(':input[name="settings[' . $this->type . '][' . $this->name . '][daemonize]"]' => array('checked' => TRUE))
      ),
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
   * Lock job.
   *
   * Background Process doesn't internally provide a unique id
   * for the running process, so we'll have to add that ourselves.
   */
  public function lock($job) {
    $handle = 'uc-' . $job->name;
    $process = new BackgroundProcess($handle);
    if (!$process->lock()) {
      return FALSE;
    }
    return $job->name . ':' . uniqid('bgpl', TRUE);
  }

  /**
   * Unlock background process.
   */
  public function unlock($lock_id, $manual = FALSE) {
    if (!preg_match('/(.*):bgpl.*/', $lock_id, $matches)) {
      watchdog('bg_process_legacy', 'Invalid lock id @lock_id', array(
        '@lock_id' => $lock_id,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
    $job_name = $matches[1];
    $handle = 'uc-' . $job_name;
    if ($manual) {
      $job = ultimate_cron_job_load($job_name);
      $job->sendSignal('background_process_legacy_dont_log');
    }
    return background_process_unlock($handle);
  }

  /**
   * Check locked state.
   *
   * Because Background Process doesn't support a unique id per
   * process, we'll have to match against the prefix, which is
   * the job name.
   */
  public function isLocked($job) {
    $process = background_process_get_process('uc-' . $job->name);
    if ($process) {
      return $process->args[1];
    }
    return FALSE;
  }

  /**
   * Check locked state for multiple jobs.
   *
   * This has yet to be optimized.
   */
  public function isLockedMultiple($jobs) {
    $handles = array();
    foreach ($jobs as $job) {
      $handles[] = 'uc-' . $job->name;
    }
    try {
      $old_db = db_set_active('background_process');
      $processes = db_select('background_process', 'bp')
        ->fields('bp', array('handle', 'args'))
        ->condition('handle', $handles, 'IN')
        ->execute()
        ->fetchAllAssoc('handle', PDO::FETCH_OBJ);
      db_set_active($old_db);
    }
    catch (Exception $e) {
      db_set_active($old_db);
      throw $e;
    }

    $lock_ids = array();
    foreach ($jobs as $job) {
      $lock_ids[$job->name] = FALSE;
      if (isset($processes['uc-' . $job->name])) {
        $process = $processes['uc-' . $job->name];
        $process->args = unserialize($process->args);
        $lock_ids[$job->name] = $process->args[1];
      }
    }

    return $lock_ids;
  }

  /**
   * Background Process launch.
   */
  public function launch($job) {
    $lock_id = $job->lock();
    if (!$lock_id) {
      return;
    }

    $settings = $job->getSettings();

    $handle = 'uc-' . $job->name;
    $process = new BackgroundProcess($handle);
    $this->exec_status = $this->status = BACKGROUND_PROCESS_STATUS_LOCKED;

    // Always run cron job as anonymous user.
    $process->uid = 0;
    $process->service_group = $settings['launcher']['background_process_legacy']['service_group'];

    $service_host = $process->determineServiceHost();

    if ($this->scheduledLaunch) {
      $init_message = t('Launched at service host @name', array(
        '@name' => $service_host,
      ));
    }
    else {
      $init_message = t('Launched manually at service host @name', array(
        '@name' => $service_host,
      ));
    }

    $log_entry = $job->startLog($lock_id, $init_message);

    // We want to finish the log in the sub-request.
    $log_entry->unCatchMessages();

    if (!$process->execute('ultimate_cron_background_process_legacy_callback', array($job->name, $lock_id))) {
      watchdog('bg_process_legacy', 'Could execute background process dispatch for handle @handle', array(
        '@handle' => $handle,
      ), WATCHDOG_ERROR);
      $this->unlock($lock_id);
      return FALSE;
    }

    drupal_set_message(t('@name: @init_message', array(
      '@name' => $job->name,
      '@init_message' => $init_message,
    )));
    return TRUE;
  }

  /**
   * Launcher cleanup.
   *
   * Nothing to do here. Background Process will handle its own cleanup.
   */
  public function cleanup() {
  }

  /**
   * Launch manager.
   */
  public function launchJobs($jobs) {
    $this->scheduledLaunch = TRUE;
    $settings = $this->getDefaultSettings();

    // Don't use more than 45 seconds for launching jobs.
    // If we fail, we will try again next time.
    $expire = microtime(TRUE) + 45;

    foreach ($jobs as $job) {
      if (!$job->schedule()) {
        continue;
      }

      // Wait until there's an available thread.
      $threads = $this->numberOfProcessesRunning();
      if ($threads >= $settings['max_threads']) {
        watchdog('bg_process_legacy', 'Background Process launcher congested. @threads/@max threads running.', array(
          '@max' => $settings['max_threads'],
          '@threads' => $threads,
        ), WATCHDOG_DEBUG);
        do {
          sleep(1);
        } while (microtime(TRUE) < $expire && $this->numberOfProcessesRunning() >= $settings['max_threads']);
      }

      // Bail out if we expired.
      if (microtime(TRUE) >= $expire) {
        watchdog('bg_process_legacy', 'Background Process launcher exceed time limit of 45 seconds.', array(), WATCHDOG_NOTICE);
        return;
      }

      // Everything's good. Launch job!
      $job->launch();
    }
  }

  /**
   * Format running state.
   */
  public function formatRunning($job) {
    $settings = $job->getSettings('launcher');
    if (empty($settings['daemonize'])) {
      return parent::formatRunning($job);
    }
    $file = drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass_go.png';
    $status = theme('image', array('path' => $file));
    $title = t('daemonized');
    return array($status, $title);
  }

  /**
   * Get the number of cron background processes currently running.
   */
  public function numberOfProcessesRunning() {
    $query = db_select('background_process', 'bp')
      ->condition('bp.handle', db_like('uc-') . '%', 'LIKE');
    $query->addExpression('COUNT(1)', 'processes');
    $result = $query
      ->execute()
      ->fetchField();
    return $result ? $result : 0;
  }
}
