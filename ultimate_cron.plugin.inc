<?php

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

  public function settingsLabel($name, $value) {
    return $value;
  }

  /**
   * Default plugin valid for all jobs.
   */
  public function isValid($job) {
    return TRUE;
  }

  /**
   * Clean form of empty fallback values.
   */
  function cleanForm($elements, &$values) {
    foreach (element_children($elements) as $child) {
      $this->cleanForm($elements[$child], $values[$child]);
      if (!empty($elements[$child]['#markup'])) {
        unset($values[$child]);
      }
      elseif (
        empty($values[$child]) &&
        !empty($elements[$child]['#fallback'])
      ) {
        unset($values[$child]);
      }
    }
  }

  /**
   * Default settings form.
   */
  static function defaultSettingsForm(&$form, &$form_state, $plugin_info) {
    $plugin_type = $plugin_info['type'];
    $static = $plugin_info['defaults']['static'];
    $key = 'ultimate_cron_default_plugin_' . $plugin_type;
    $options = array();
    foreach (ultimate_cron_plugin_load_all($plugin_type) as $name => $plugin) {
      $options[$name] = $plugin->title;
    }
    $form[$key] = array(
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => variable_get($key, $static['default plugin']),
      '#title' => t('Default @plugin_type', array('@plugin_type' => $static['title singular'])),
    );
    $form = system_settings_form($form);
  }

  static function jobSettingsForm(&$form, &$form_state, $plugin_type, $job) {
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
    $form_state['values']['settings'][$plugin_type] += array($current_plugin->name => array());

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
    $options[0] = t('Default (@default)', array(
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
      '#title'         => t(ucfirst($plugin_type)),
      '#description'   => t(ucfirst($plugin_type) . ' for this job.'),
      '#ajax' => array(
        'callback' => 'ultimate_cron_job_plugin_settings_ajax',
        'wrapper' => $wrapper,
        'method' => 'replace',
        'effect' => 'none',
      ),
    );

    // Plugin specific settings wrapper for ajax replace.
    $form['settings'][$plugin_type][$current_plugin->name] = array(
      '#tree' => TRUE,
      '#prefix' => '<div id="' . $wrapper . '">',
      '#suffix' => '</div>',
    );

    $form_state['default_values']['settings'][$plugin_type][$current_plugin->name] = $defaults;
    $form_state['values']['settings'][$plugin_type][$current_plugin->name] += ultimate_cron_blank_values($defaults);

    $current_plugin->settingsForm($form, $form_state, $job);
    if (empty($form['settings'][$plugin_type][$current_plugin->name]['no_settings'])) {
      $current_plugin->fallbackalize($form['settings'][$plugin_type][$current_plugin->name], $form_state['default_values']['settings'][$plugin_type][$current_plugin->name], FALSE);
    }
  }

  static function jobSettingsFormValidate($form, &$form_state, $plugin_type, $job) {
    $name = !empty($form_state['values']['settings'][$plugin_type]['name']) ? $form_state['values']['settings'][$plugin_type]['name'] : $job->hook[$plugin_type]['name'];
    $plugin = ultimate_cron_plugin_load($plugin_type, $name);
    $plugin->settingsFormValidate($form, $form_state);
  }

  static function jobSettingsFormSubmit($form, &$form_state, $plugin_type, $job) {
    $name = !empty($form_state['values']['settings'][$plugin_type]['name']) ? $form_state['values']['settings'][$plugin_type]['name'] : $job->hook[$plugin_type]['name'];
    $plugin = ultimate_cron_plugin_load($plugin_type, $name);
    $plugin->settingsFormSubmit($form, $form_state);

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
  public function settingsFormValidate(&$form, &$form_state) {
  }

  /**
   * Settings form submit handler.
   */
  public function settingsFormSubmit(&$form, &$form_state) {
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
  function fallbackalize(&$elements, $defaults, $remove_non_fallbacks = FALSE) {
    if (empty($elements)) {
      return;
    }
    foreach (element_children($elements) as $child) {
      $element = &$elements[$child];
      $this->fallbackalize($element, $defaults[$child], $remove_non_fallbacks);

      if (!empty($element['#fallback'])) {
        if (!$remove_non_fallbacks) {
          if ($element['#type'] == 'select' && empty($element['#multiple'])) {
            $element['#options'] = array(
              '0' => t('Default (@default)', array('@default' => $this->settingsLabel($child, $defaults[$child]))),
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
    }
  }
}

class UltimateCronPluginMultiple extends UltimateCronPlugin {
  static public $multiple = TRUE;

  /**
   * Default settings form.
   */
  static function defaultSettingsForm(&$form, &$form_state, $plugin_info) {
    $plugin_type = $plugin_info['type'];
    foreach (ultimate_cron_plugin_load_all($plugin_type) as $name => $plugin) {
      $plugins[] = l($plugin->title, "admin/config/system/cron/$plugin_type/$name");
    }
    $form['available'] = array(
      '#markup' => theme('item_list', array(
        'title' => $plugin_info['defaults']['static']['title plural proper'] . ' available',
        'items' => $plugins
      ))
    );
  }

  static function jobSettingsForm(&$form, &$form_state, $plugin_type, $job) {
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
    foreach ($plugins as $name => $plugin) {
      $form_state['default_values']['settings'][$plugin_type] += array($name => array());
      $form_state['values']['settings'][$plugin_type] += array($name => array());
      $form['settings'][$plugin_type][$name] = array(
        '#title' => $plugin->title,
        '#group' => 'settings_tabs',
        '#type' => 'fieldset',
        // '#tree' => TRUE,
        '#visible' => TRUE,
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#weight' => $weight++,
      );

      $defaults = $plugin->getDefaultSettings($job);

      $form_state['default_values']['settings'][$plugin_type][$name] += $defaults;
      $form_state['values']['settings'][$plugin_type][$name] += ultimate_cron_blank_values($defaults);

      $plugin->settingsForm($form, $form_state, $job);
      $plugin->fallbackalize($form['settings'][$plugin_type][$name], $form_state['default_values']['settings'][$plugin_type][$name], FALSE);
    }
  }

  static function jobSettingsFormValidate($form, &$form_state, $plugin_type, $job) {
    $plugins = ultimate_cron_plugin_load_all($plugin_type);
    foreach ($plugins as $name => $plugin) {
      if ($plugin->isValid($job)) {
        $plugin->settingsFormValidate($form, $form_state);
      }
    }
  }

  static function jobSettingsFormSubmit($form, &$form_state, $plugin_type, $job) {
    $plugins = ultimate_cron_plugin_load_all($plugin_type);
    foreach ($plugins as $name => $plugin) {
      if ($plugin->isValid($job)) {
        $plugin->settingsFormSubmit($form, $form_state);

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
  abstract public function schedule($job);
}

abstract class UltimateCronLauncher extends UltimateCronPlugin {
  /**
   * Launcher.
   */
  public function launch($job) {
    // Run job.
    try {
      $job->run();
    }
    catch (Exception $e) {
      watchdog('ultimate_cron', 'Error executing %job: @error', array('%job' => $job->name, '@error' => $e->getMessage()), WATCHDOG_ERROR);
    }
  }

  static public function launchJobs($jobs) {
    dpm("Default launch");
    foreach ($jobs as $job) {
      $job->launch();
    }
  }

}

abstract class UltimateCronLogger extends UltimateCronPlugin {
  public $name;
  public $module;
  public $finished = FALSE;

  public $lid = NULL;
  public $start_time = 0;
  public $end_time = 0;
  public $message = '';
  public $severity = -1;
  static public $logs = array();

  function initialize($job) {
    $this->lid = NULL;
    $this->start_time = microtime(TRUE);
    $this->end_time = 0;
    $this->message = '';
    $this->severity = -1;

    $this->module = $job->hook['module'];
    $this->name = $job->name;
    $this->save();
  }

  function watchdog(array $log_entry) {
    $this->message .= t($log_entry['message'], $log_entry['variables']) . "\n";
    if ($this->severity < 0 || $this->severity > $log_entry['severity']) {
      $this->severity = $log_entry['severity'];
    }
  }

  function catchMessages() {
    self::$logs[$this->lid] = $this;
    drupal_register_shutdown_function(array($this, 'catchMessagesShutdown'));
  }

  function unCatchMessages() {
    unset(self::$logs[$this->lid]);
  }

  static function hook_watchdog(array $log_entry) {
    foreach (self::$logs as $log) {
      $log->watchdog($log_entry);
    }
  }

  public function catchMessagesShutdown() {
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
        'type'        => $this->module,
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
