<?php
/**
 * @file
 * Background Process 1.x launcher for Ultimate Cron.
 */

/**
 * Ultimate Cron launcher plugin class.
 */
class UltimateCronBackgroundProcessLegacyLauncher extends UltimateCronLauncher {
  static private $currentThread = NULL;

  /**
   * Use ajax for run, since we're launching in the background.
   */
  public function build_operations_alter($job, &$allowed_operations) {
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
   * Lock job.
   *
   * Background Process doesn't internally provide a unique id
   * for the running process, so we'll have to add that ourselves.
   */
  public function lock($job) {
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
  public function unlock($lock_id) {
    return background_process_unlock($lock_id);
  }

  /**
   * Check locked state.
   *
   * Because Background Process doesn't support a unique id per
   * process, we'll have to match against the prefix, which is
   * the job name.
   */
  public function isLocked($job) {
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
  public function launch($job) {
    $settings = $job->getSettings();

    $lock_id = $job->lock();

    $process = new BackgroundProcess($lock_id);
    $this->exec_status = $this->status = BACKGROUND_PROCESS_STATUS_LOCKED;

    // Always run cron job as anonymous user.
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
      $username = $user->uid ? $user->name : t('Anonymous');
      $init_message = t('Launched at service host @name by @username', array(
        '@name' => $service_host,
        '@username' => $username,
      ));
    }

    $log = $job->startLog($lock_id, $init_message);

    // We want to finish the log in the sub-request.
    $log->unCatchMessages();

    $process->execute('ultimate_cron_background_process_legacy_callback', array($job->name, $log->log_entry->lid));

    drupal_set_message(t('@name: @init_message', array(
      '@name' => $job->name,
      '@init_message' => $init_message,
    )));
  }

  /**
   * Launch manager.
   */
  static public function launchJobs($jobs) {
    self::$currentThread = 1;
    foreach ($jobs as $job) {
      $job->launch();
    }
  }
}
