<?php
/**
 * @file
 * Plugin framework for Ultimate Cron.
 */

class UltimateCronPlugin {
  public $name = '';
  public $title = '';
  public $description = '';
  public $plugin;
  public $settings = array();
  static public $multiple = FALSE;

  /**
   * Constructor.
   *
   * Setup object.
   *
   * @param string $name
   *   Name of plugin.
   * @param array $plugin
   *   The plugin definition.
   */
  public function __construct($name, $plugin) {
    $this->plugin = $plugin;
    $this->title = $plugin['title'];
    $this->description = $plugin['description'];
    $this->name = $name;
    $this->type = $plugin['plugin type'];
    $this->key = 'ultimate_cron_plugin_' . $plugin['plugin type'] . '_' . $name . '_settings';
    $this->settings = variable_get($this->key, array());
  }

  /**
   * Invoke hook_cron_alter() on plugins.
   */
  final static public function hook_cron_alter(&$jobs) {
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid()) {
          $plugin->cron_alter($jobs);
        }
      }
    }
  }

  /**
   * hook_cron_alter() for plugins.
   */
  public function cron_alter($jobs) {
  }

  /**
   * Custom action for plugins.
   */
  public function custom_page($js, $input, $item, $action) {
  }

  /**
   * Allow plugins to alter the allowed operations for a job.
   */
  public function build_operations_alter($job, &$allowed_operations) {
  }

  /**
   * Get default settings.
   */
  public function getDefaultSettings($job = NULL) {
    $settings = array();
    if ($job && !empty($job->hook[$this->type][$this->name])) {
      $settings += $job->hook[$this->type][$this->name];
    }
    $settings += $this->settings + $this->defaultSettings();
    return $settings;
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array();
  }

  /**
   * Get label for a specific setting.
   */
  public function settingsLabel($name, $value) {
    if (is_array($value)) {
      return implode(', ', $value);
    }
    else {
      return $value;
    }
  }

  /**
   * Default plugin valid for all jobs.
   */
  public function isValid($job = NULL) {
    return TRUE;
  }

  public function cleanup() {
  }

  /**
   * Clean form of empty fallback values.
   */
  public function cleanForm($elements, &$values) {
    if (empty($elements)) {
      return;
    }
    foreach (element_children($elements) as $child) {
      if (empty($child) || empty($elements[$child]) || is_numeric($child)) {
        continue;
      }
      if (empty($element['#tree'])) {
        $param_values = &$values;
      }
      else {
        $param_values = &$values[$child];
      }
      $this->cleanForm($elements[$child], $param_values);
      if (!empty($elements[$child]['#markup'])) {
        unset($values[$child]);
      }
      elseif (
        isset($values[$child]) &&
        !$values[$child] &&
        !empty($elements[$child]['#fallback']) &&
        $values[$child] !== '0'
      ) {
        unset($values[$child]);
      }
    }
  }

  /**
   * Default settings form.
   */
  static public function defaultSettingsForm(&$form, &$form_state, $plugin_info) {
    $plugin_type = $plugin_info['type'];
    $static = $plugin_info['defaults']['static'];
    $key = 'ultimate_cron_plugin_' . $plugin_type . '_default';
    $options = array();
    foreach (ultimate_cron_plugin_load_all($plugin_type) as $name => $plugin) {
      if ($plugin->isValid()) {
        $options[$name] = $plugin->title;
      }
    }
    $form[$key] = array(
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => variable_get($key, $static['default plugin']),
      '#title' => t('Default @plugin_type', array('@plugin_type' => $static['title singular'])),
    );
    $form = system_settings_form($form);
  }

  /**
   * Job settings form.
   */
  static public function jobSettingsForm(&$form, &$form_state, $plugin_type, $job) {
    // Check valid plugins.
    $plugins = ultimate_cron_plugin_load_all($plugin_type);
    foreach ($plugins as $name => $plugin) {
      if (!$plugin->isValid($job)) {
        unset($plugins[$name]);
      }
    }

    // No plugins = no settings = no vertical tabs for you mister!
    if (empty($plugins)) {
      continue;
    }

    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    $plugin_info = $plugin_types['ultimate_cron'][$plugin_type];
    $static = $plugin_info['defaults']['static'];

    // Find plugin selected on this page.
    // If "0" (meaning default) use the one defined in the hook.
    if (empty($form_state['values']['settings'][$plugin_type]['name'])) {
      $form_state['values']['settings'][$plugin_type]['name'] = 0;
      $current_plugin = $plugins[$job->hook[$plugin_type]['name']];
    }
    else {
      $current_plugin = $plugins[$form_state['values']['settings'][$plugin_type]['name']];
    }
    $form_state['previous_plugin'][$plugin_type] = $current_plugin->name;

    // Determine original plugin.
    $original_plugin = !empty($job->settings[$plugin_type]['name']) ? $job->settings[$plugin_type]['name'] : $job->hook[$plugin_type]['name'];

    // Ensure blank array.
    if (empty($form_state['values']['settings'][$plugin_type][$current_plugin->name])) {
      $form_state['values']['settings'][$plugin_type][$current_plugin->name] = array();
    }

    // Default values for current selection. If selection differs from current job, then
    // take the job (or hook) into account.
    $defaults = $current_plugin->name == $original_plugin ? $current_plugin->getDefaultSettings($job) : $current_plugin->getDefaultSettings();

    // Plugin settings fieldset with vertical tab reference.
    $form['settings'][$plugin_type] = array(
      '#type' => 'fieldset',
      '#title' => $static['title singular proper'],
      '#group' => 'settings_tabs',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    // Ajax wrapper.
    $wrapper = 'wrapper-plugin-' . $plugin_type . '-settings';

    // Setup plugin selector.
    $options = array();
    $options[''] = t('Default (@default)', array(
      '@default' => $plugins[$job->hook[$plugin_type]['name']]->title,
    ));
    foreach ($plugins as $name => $plugin) {
      $options[$name] = $plugin->title;
    }
    $form['settings'][$plugin_type]['name'] = array(
      '#weight'        => -10,
      '#type'          => 'select',
      '#options'       => $options,
      '#default_value' => $form_state['values']['settings'][$plugin_type]['name'],
      '#title'         => $static['title singular proper'],
      '#description'   => t('Select which @plugin to use for this job.', array(
        '@plugin' => $static['title singular'],
      )),
      '#ajax' => array(
        'callback' => 'ultimate_cron_job_plugin_settings_ajax',
        'wrapper' => $wrapper,
        'method' => 'replace',
        'effect' => 'none',
      ),
    );

    $default_settings_link = l(
      t('(change default settings)'),
      'admin/config/system/cron/' . $current_plugin->type . '/' . $current_plugin->name
    );

    // Plugin specific settings wrapper for ajax replace.
    $form['settings'][$plugin_type][$current_plugin->name] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => $current_plugin->title,
      '#description' => $current_plugin->description,
      '#prefix' => '<div id="' . $wrapper . '">',
      '#suffix' => '</div>',
    );

    $form_state['default_values']['settings'][$plugin_type][$current_plugin->name] = $defaults;
    $form_state['values']['settings'][$plugin_type][$current_plugin->name] += ultimate_cron_blank_values($defaults);

    $current_plugin->settingsForm($form, $form_state, $job);
    if (empty($form['settings'][$plugin_type][$current_plugin->name]['no_settings'])) {
      $current_plugin->fallbackalize(
        $form['settings'][$plugin_type][$current_plugin->name],
        $form_state['values']['settings'][$plugin_type][$current_plugin->name],
        $form_state['default_values']['settings'][$plugin_type][$current_plugin->name],
        FALSE
      );
      $form['settings'][$plugin_type][$current_plugin->name]['#description'] .= ' ' . $default_settings_link;
    }
  }

  /**
   * Job settings form validate handler.
   */
  static public function jobSettingsFormValidate($form, &$form_state, $plugin_type, $job = NULL) {
    $name = !empty($form_state['values']['settings'][$plugin_type]['name']) ? $form_state['values']['settings'][$plugin_type]['name'] : $job->hook[$plugin_type]['name'];
    $plugin = ultimate_cron_plugin_load($plugin_type, $name);
    $plugin->settingsFormValidate($form, $form_state, $job);
  }

  /**
   * Job settings form submit handler.
   */
  static public function jobSettingsFormSubmit($form, &$form_state, $plugin_type, $job = NULL) {
    $name = !empty($form_state['values']['settings'][$plugin_type]['name']) ? $form_state['values']['settings'][$plugin_type]['name'] : $job->hook[$plugin_type]['name'];
    $plugin = ultimate_cron_plugin_load($plugin_type, $name);
    $plugin->settingsFormSubmit($form, $form_state, $job);

    // Weed out blank values that have fallbacks.
    $elements = &$form['settings'][$plugin_type][$name];
    $values = &$form_state['values']['settings'][$plugin_type][$name];;
    $plugin->cleanForm($elements, $values);
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $form['settings'][$this->type][$this->name]['no_settings'] = array(
      '#markup' => '<p>' . t('This plugin has no settings.') . '</p>',
    );
  }

  /**
   * Settings form validate handler.
   */
  public function settingsFormValidate(&$form, &$form_state, $job = NULL) {
  }

  /**
   * Settings form submit handler.
   */
  public function settingsFormSubmit(&$form, &$form_state, $job = NULL) {
  }

  /**
   * Process fallback form parameters.
   *
   * @param array $elements
   *   Elements to process.
   * @param array $defaults
   *   Default values to add to description.
   * @param boolean $remove_non_fallbacks
   *   If TRUE, non fallback elements will be removed.
   */
  public function fallbackalize(&$elements, &$values, $defaults, $remove_non_fallbacks = FALSE) {
    if (empty($elements)) {
      return;
    }
    foreach (element_children($elements) as $child) {
      $element = &$elements[$child];
      if (empty($element['#tree'])) {
        $param_values = &$values;
        $param_defaults = &$defaults;
      }
      else {
        $param_values = &$values[$child];
        $param_defaults = &$defaults[$child];
      }
      $this->fallbackalize($element, $param_values, $param_defaults, $remove_non_fallbacks);

      if (empty($element['#type']) || $element['#type'] == 'fieldset') {
        continue;
      }

      if (!empty($element['#fallback'])) {
        if (!$remove_non_fallbacks) {
          if ($element['#type'] == 'radios') {
            $label = $this->settingsLabel($child, $defaults[$child]);
            $element['#options'] = array(
              '' => t('Default (@default)', array('@default' => $label)),
            ) + $element['#options'];
          }
          elseif ($element['#type'] == 'select' && empty($element['#multiple'])) {
            $label = $this->settingsLabel($child, $defaults[$child]);
            $element['#options'] = array(
              '' => t('Default (@default)', array('@default' => $label)),
            ) + $element['#options'];
          }
          elseif ($defaults[$child] !== '') {
            $element['#description'] .= ' ' . t('(Blank = @default)', array('@default' => $this->settingsLabel($child, $defaults[$child])));
          }
          unset($element['#required']);
        }
      }
      elseif (!empty($element['#type']) && $remove_non_fallbacks) {
        unset($elements[$child]);
      }
      elseif (!isset($element['#default_value']) || $element['#default_value'] === '') {
        $values[$child] = $defaults[$child];
        $element['#default_value'] = $defaults[$child];
      }
    }
  }
}

class UltimateCronPluginMultiple extends UltimateCronPlugin {
  static public $multiple = TRUE;

  /**
   * Default settings form.
   */
  static public function defaultSettingsForm(&$form, &$form_state, $plugin_info) {
    $plugin_type = $plugin_info['type'];
    foreach (ultimate_cron_plugin_load_all($plugin_type) as $name => $plugin) {
      if ($plugin->isValid()) {
        $plugins[] = l($plugin->title, "admin/config/system/cron/$plugin_type/$name");
      }
    }
    $form['available'] = array(
      '#markup' => theme('item_list', array(
        'title' => $plugin_info['defaults']['static']['title plural proper'] . ' available',
        'items' => $plugins
      ))
    );
  }

  /**
   * Job settings form.
   */
  static public function jobSettingsForm(&$form, &$form_state, $plugin_type, $job) {
    // Check valid plugins.
    $plugins = ultimate_cron_plugin_load_all($plugin_type);
    foreach ($plugins as $name => $plugin) {
      if (!$plugin->isValid($job)) {
        unset($plugins[$name]);
      }
    }

    // No plugins = no settings = no vertical tabs for you mister!
    if (empty($plugins)) {
      continue;
    }

    $weight = 10;
    $form_state['default_values']['settings'][$plugin_type] = array();
    $form['settings'][$plugin_type]['#tree'] = TRUE;
    foreach ($plugins as $name => $plugin) {
      $form_state['default_values']['settings'][$plugin_type] += array($name => array());
      $form_state['values']['settings'][$plugin_type] += array($name => array());
      $form['settings'][$plugin_type][$name] = array(
        '#title' => $plugin->title,
        '#group' => 'settings_tabs',
        '#type' => 'fieldset',
        '#tree' => TRUE,
        '#visible' => TRUE,
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#weight' => $weight++,
      );

      $defaults = $plugin->getDefaultSettings($job);

      $form_state['default_values']['settings'][$plugin_type][$name] += $defaults;
      $form_state['values']['settings'][$plugin_type][$name] += ultimate_cron_blank_values($defaults);

      $plugin->settingsForm($form, $form_state, $job);
      if (empty($form['settings'][$plugin_type][$name]['no_settings'])) {
        $plugin->fallbackalize(
          $form['settings'][$plugin_type][$name],
          $form_state['values']['settings'][$plugin_type][$name],
          $form_state['default_values']['settings'][$plugin_type][$name],
          FALSE
        );
      }
      else {
        unset($form['settings'][$plugin_type][$name]);
      }
    }
  }

  /**
   * Job settings form validate handler.
   */
  static public function jobSettingsFormValidate($form, &$form_state, $plugin_type, $job) {
    $plugins = ultimate_cron_plugin_load_all($plugin_type);
    foreach ($plugins as $name => $plugin) {
      if ($plugin->isValid($job)) {
        $plugin->settingsFormValidate($form, $form_state, $job);
      }
    }
  }

  /**
   * Job settings form submit handler.
   */
  static public function jobSettingsFormSubmit($form, &$form_state, $plugin_type, $job) {
    $plugins = ultimate_cron_plugin_load_all($plugin_type);
    foreach ($plugins as $name => $plugin) {
      if ($plugin->isValid($job)) {
        $plugin->settingsFormSubmit($form, $form_state, $job);

        // Weed out blank values that have fallbacks.
        $elements = &$form['settings'][$plugin_type][$name];
        $values = &$form_state['values']['settings'][$plugin_type][$name];
        $plugin->cleanForm($elements, $values);
      }
      else {
        unset($form_state['values']['settings'][$plugin_type][$name]);
      }
    }
  }
}

abstract class UltimateCronScheduler extends UltimateCronPlugin {
  /**
   * Check job schedule.
   *
   * @param UltimateCronJob $job
   *   The job to check schedule for.
   *
   * @return bool
   *   TRUE if job should run.
   */
  abstract public function schedule($job);
}

abstract class UltimateCronLauncher extends UltimateCronPlugin {
  /**
   * Launch job.
   */
  abstract public function launch($job);

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'lock_timeout' => 3600,
    ) + parent::defaultSettings();
  }

  /**
   * Generate lock key for job.
   */
  public function lockKey($job) {
    return $job->name;
  }

  /**
   * Lock job.
   */
  public function lock($job) {
    $settings = $job->getSettings($this->type);
    $timeout = $settings['lock_timeout'];

    if ($lock_id = UltimateCronLock::lock($this->lockKey($job), $timeout)) {
      $lock_id = $this->name . '-' . $lock_id;
      return $lock_id;
    }
    return FALSE;
  }

  /**
   * Unlock job.
   */
  public function unlock($lock_id, $manual = FALSE) {
    list($launcher, $lock_id) = explode('-', $lock_id, 2);
    return UltimateCronLock::unlock($lock_id);
  }

  /**
   * Check if job is locked.
   */
  public function isLocked($job) {
    $lock_id = UltimateCronLock::isLocked($this->lockKey($job));
    return $lock_id ? $this->name . '-' . $lock_id : $lock_id;
  }

  /**
   * Run the job.
   */
  public function run($job) {
    $php_self = NULL;
    try {
      // Signal to whomever might be listening, that we're cron!
      // @investigate Is this safe? (He asked knowingly ...)
      $php_self = $_SERVER['PHP_SELF'] ? $_SERVER['PHP_SELF'] : '';
      $_SERVER['PHP_SELF'] = 'cron.php';

      $job->invoke();

      // Restore state.
      $_SERVER['PHP_SELF'] = $php_self;
    }
    catch (Exception $e) {
      // Restore state.
      if (isset($php_self)) {
        $_SERVER['PHP_SELF'] = $php_self;
      }

      watchdog('ultimate_cron', 'Error running @name: @error', array('@error' => $e->getMessage()), WATCHDOG_ERROR);
    }
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['lock_timeout'] = array(
      '#title' => t("Job lock timeout"),
      '#type' => 'textfield',
      '#default_value' => $values['lock_timeout'],
      '#description' => t('Number of seconds to keep lock on job.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }

  /**
   * Launch scheduled cron jobs.
   */
  static public function launchJobs($jobs) {
    foreach ($jobs as $job) {
      $job->launch();
    }
  }

  /**
   * Launch scheduled cron jobs.
   */
  static public function launchPoorman($class, $jobs) {
    $settings = ultimate_cron_plugin_load('settings', 'general')->getDefaultSettings();
    $cron_last = variable_get('cron_last', 0);
    $time = time();
    if ($settings['poorman_interval'] && ($cron_last < time() - $settings['poorman_interval'])) {
      error_log("LAUNCHING POORMAN: $class");
      // return $class::launchJobs($jobs, TRUE);
    }
  }

  function cleanup() {
    UltimateCronLock::cleanup();
  }
}

abstract class UltimateCronLogger extends UltimateCronPlugin {
  public $finished = FALSE;

  public $log_entry = NULL;

  static public $logs = array();

  /**
   * Constructor.
   */
  public function __construct($name, $plugin) {
    global $user;
    parent::__construct($name, $plugin);
    $this->log_entry = new stdClass();
    $this->log_entry->lid = NULL;
    $this->log_entry->uid = $user->uid;
    $this->log_entry->start_time = 0;
    $this->log_entry->end_time = 0;
    $this->log_entry->init_message = '';
    $this->log_entry->message = '';
    $this->log_entry->severity = -1;
  }

  /**
   * Initialize a new log entry.
   */
  public function initialize($lock_id, $job, $init_message = '') {
    global $user;

    $this->log_entry = new stdClass();
    $this->log_entry->lid = $lock_id;
    $this->log_entry->uid = $user->uid;
    $this->log_entry->start_time = microtime(TRUE);
    $this->log_entry->end_time = 0;
    $this->log_entry->init_message = $init_message;
    $this->log_entry->message = '';
    $this->log_entry->severity = -1;

    $this->log_entry->module = $job->hook['module'];
    $this->log_entry->name = $job->name;
    $this->job = $job;
    $this->save();
  }

  /**
   * Finish a log and save it if applicable.
   */
  public function finish() {
    if (!$this->finished) {
      $this->unCatchMessages();
      $this->log_entry->end_time = microtime(TRUE);
      $this->finished = TRUE;
      $this->save();
    }
  }

  /**
   * Implements hook_watchdog().
   */
  public function watchdog(array $log_entry) {
    $this->log_entry->message .= t($log_entry['message'], $log_entry['variables']) . "\n";
    if ($this->log_entry->severity < 0 || $this->log_entry->severity > $log_entry['severity']) {
      $this->log_entry->severity = $log_entry['severity'];
    }
    // Make sure that message don't become too big.
    $size = 2000;
    if (mb_strlen($this->log_entry->message) > $size) {
      while (mb_strlen($this->log_entry->message) > $size) {
        $firstline = strpos($this->log_entry->message, "\n");
        if ($firstline === FALSE) {
          // Only one line? That's a big line ... truncate it!
          $this->log_entry->message = mb_substr($this->log_entry->message, -$size);
          break;
        }
        $this->log_entry->message = substr($this->log_entry->message, $firstline + 1);
      }
      $this->log_entry->message = '.....' . $this->log_entry->message;
    }
  }

  /**
   * Begin capturing messages.
   */
  public function catchMessages() {
    $class = get_class($this);
    if (empty($class::$logs)) {
      // Since we may already be inside a drupal_register_shutdown_function()
      // we cannot use that. Use PHPs register_shutdown_function() instead.
      register_shutdown_function(array($class, 'catchMessagesShutdownWrapper'), $class);
    }
    $class::$logs[$this->log_entry->lid] = $this;
  }

  /**
   * End message capturing.
   */
  public function unCatchMessages() {
    $class = get_class($this);
    unset($class::$logs[$this->log_entry->lid]);
  }

  /**
   * Check if we're already catching messages.
   */
  public function isCatched() {
    $class = get_class($this);
    return !empty($class::$logs[$this->log_entry->lid]);
  }

  /**
   * Invoke loggers watchdog hooks.
   */
  final static public function hook_watchdog(array $log_entry) {
    foreach (self::$logs as $log) {
      $log->watchdog($log_entry);
    }
  }

  /**
   * Shutdown handler wrapper for catching messages.
   */
  static public function catchMessagesShutdownWrapper($class) {
    if ($class::$logs) {
      foreach ($class::$logs as $lid => $log) {
        $log->catchMessagesShutdown();
      }
    }
  }

  /**
   * PHP shutdown function callback.
   *
   * Log errors if any.
   */
  public function catchMessagesShutdown() {
    $class = get_class($this);
    unset($class::$logs[$this->log_entry->lid]);

    if ($this->finished) {
      return;
    }

    // Get error messages.
    $error = error_get_last();
    if ($error) {
      $message = $error['message'] . ' (line ' . $error['line'] . ' of ' . $error['file'] . ').' . "\n";
      $severity = WATCHDOG_INFO;
      if ($error['type'] && (E_NOTICE || E_USER_NOTICE || E_USER_WARNING)) {
        $severity = WATCHDOG_NOTICE;
      }
      if ($error['type'] && (E_WARNING || E_CORE_WARNING || E_USER_WARNING)) {
        $severity = WATCHDOG_WARNING;
      }
      if ($error['type'] && (E_ERROR || E_CORE_ERROR || E_USER_ERROR || E_RECOVERABLE_ERROR)) {
        $severity = WATCHDOG_ERROR;
      }

      global $user;
      global $base_root;
      $log_entry = array(
        'type'        => $this->log_entry->module,
        'message'     => $message,
        'variables'   => array(),
        'severity'    => $severity,
        'link'        => NULL,
        'user'        => $user,
        'uid'         => isset($user->uid) ? $user->uid : 0,
        'request_uri' => $base_root . request_uri(),
        'referer'     => isset($_SERVER['HTTP_REFERER']) ? $_SERVER['HTTP_REFERER'] : '',
        'ip'          => ip_address(),
        // Request time isn't accurate for long processes, use time() instead.
        'timestamp'   => time(),
      );

      $this->watchdog($log_entry);
    }
    $this->finish();
  }
}

class UltimateCronSettings extends UltimateCronPluginMultiple {
}

class UltimateCronTaggedSettings extends UltimateCronSettings {
  /**
   * Only valid for jobs tagged with the proper tag.
   */
  public function isValid($job = NULL) {
    return $job ? in_array($this->name, $job->hook['tags']) : parent::isValid();
  }
}
