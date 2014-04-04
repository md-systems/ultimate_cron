<?php
/**
 * Created by PhpStorm.
 * User: berdir
 * Date: 4/4/14
 * Time: 3:01 PM
 */


namespace Drupal\ultimate_cron;
use Drupal\Core\Plugin\PluginBase;

/**
 * This is the base class for all Ultimate Cron plugins.
 *
 * This class handles all the load/save settings for a plugin as well as the
 * forms, etc.
 */
class CronPlugin extends PluginBase {
  public $plugin;
  public $settings = array();
  static public $multiple = FALSE;
  static public $instances = array();
  public $weight = 0;
  static public $globalOptions = array();

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
  /*public function __construct($name, $plugin) {
    $this->plugin = $plugin;
    $this->title = $plugin['title'];
    $this->description = $plugin['description'];
    $this->name = $name;
    $this->type = $plugin['plugin type'];
    $this->key = 'ultimate_cron_plugin_' . $plugin['plugin type'] . '_' . $name . '_settings';
    $this->settings = variable_get($this->key, array());
  }*/

  /**
   * Get global plugin option.
   *
   * @param string $name
   *   Name of global plugin option to get.
   *
   * @return mixed
   *   Value of option if any, NULL if not found.
   */
  static public function getGlobalOption($name) {
    return isset(static::$globalOptions[$name]) ? static::$globalOptions[$name] : NULL;
  }

  /**
   * Get all global plugin options.
   *
   * @return array
   *   All options currently set, keyed by name.
   */
  static public function getGlobalOptions() {
    return static::$globalOptions;
  }

  /**
   * Set global plugin option.
   *
   * @param string $name
   *   Name of global plugin option to get.
   * @param string $value
   *   The value to give it.
   */
  static public function setGlobalOption($name, $value) {
    static::$globalOptions[$name] = $value;
  }

  /**
   * Remove a global plugin option.
   *
   * @param string $name
   *   Name of global plugin option to remove.
   */
  static public function unsetGlobalOption($name) {
    unset(static::$globalOptions[$name]);
  }

  /**
   * Remove all global plugin options.
   */
  static public function unsetGlobalOptions() {
    static::$globalOptions = array();
  }

  /**
   * Invoke hook_cron_alter() on plugins.
   */
  final static public function hook_cron_alter(&$jobs) {
    ctools_include('plugins');
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
   * Invoke hook_cron_pre_schedule() on plugins.
   */
  final static public function hook_cron_pre_schedule($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_pre_schedule($job);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_post_schedule() on plugins.
   */
  final static public function hook_cron_post_schedule($job, &$result) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_post_schedule($job, $result);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_pre_launch() on plugins.
   */
  final static public function hook_cron_pre_launch($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_pre_launch($job);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_post_launch() on plugins.
   */
  final static public function hook_cron_post_launch($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_post_launch($job);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_pre_run() on plugins.
   */
  final static public function hook_cron_pre_run($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_pre_run($job);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_post_run() on plugins.
   */
  final static public function hook_cron_post_run($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_post_run($job);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_pre_invoke() on plugins.
   */
  final static public function hook_cron_pre_invoke($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_pre_invoke($job);
        }
      }
    }
  }

  /**
   * Invoke hook_cron_post_invoke() on plugins.
   */
  final static public function hook_cron_post_invoke($job) {
    ctools_include('plugins');
    $plugin_types = ctools_plugin_get_plugin_type_info();
    foreach ($plugin_types['ultimate_cron'] as $plugin_type => $info) {
      $plugins = ultimate_cron_plugin_load_all($plugin_type);
      foreach ($plugins as $plugin) {
        if ($plugin->isValid($job)) {
          $plugin->cron_post_invoke($job);
        }
      }
    }
  }

  /**
   * A hook_cronapi() for plugins.
   */
  public function cronapi() {
    return array();
  }

  /**
   * A hook_cron_alter() for plugins.
   */
  public function cron_alter(&$jobs) {
  }

  /**
   * A hook_cron_pre_schedule() for plugins.
   */
  public function cron_pre_schedule($job) {
  }

  /**
   * A hook_cron_post_schedule() for plugins.
   */
  public function cron_post_schedule($job, &$result) {
  }

  /**
   * A hook_cron_pre_launch() for plugins.
   */
  public function cron_pre_launch($job) {
  }

  /**
   * A hook_cron_post_launch() for plugins.
   */
  public function cron_post_launch($job) {
  }

  /**
   * A hook_cron_pre_run() for plugins.
   */
  public function cron_pre_run($job) {
  }

  /**
   * A hook_cron_post_run() for plugins.
   */
  public function cron_post_run($job) {
  }

  /**
   * A hook_cron_pre_invoke() for plugins.
   */
  public function cron_pre_invoke($job) {
  }

  /**
   * A hook_cron_post_invoke() for plugins.
   */
  public function cron_post_invoke($job) {
  }

  /**
   * Signal page for plugins.
   */
  public function signal($item, $signal) {
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
   * Save settings to db.
   */
  public function setSettings() {
    variable_set($this->key, $this->settings);
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
   * Format label for the plugin.
   *
   * @param CronJob $job
   *   The job for format the plugin label for.
   *
   * @return string
   *   Formatted label.
   */
  public function formatLabel($job) {
    return $job->name;
  }

  /**
   * Format verbose label for the plugin.
   *
   * @param CronJob $job
   *   The job for format the verbose plugin label for.
   *
   * @return string
   *   Verbosely formatted label.
   */
  public function formatLabelVerbose($job) {
    return $job->title;
  }

  /**
   * Default plugin valid for all jobs.
   */
  public function isValid($job = NULL) {
    return TRUE;
  }

  /**
   * Modified version drupal_array_get_nested_value().
   *
   * Removes the specified parents leaf from the array.
   *
   * @param array $array
   *   Nested associative array.
   * @param array $parents
   *   Array of key names forming a "path" where the leaf will be removed
   *   from $array.
   */
  public function drupal_array_remove_nested_value(array &$array, array $parents) {
    $ref = & $array;
    $last_parent = array_pop($parents);
    foreach ($parents as $parent) {
      if (is_array($ref) && array_key_exists($parent, $ref)) {
        $ref = & $ref[$parent];
      }
      else {
        return;
      }
    }
    unset($ref[$last_parent]);
  }

  /**
   * Clean form of empty fallback values.
   */
  public function cleanForm($elements, &$values, $parents) {
    if (empty($elements)) {
      return;
    }

    foreach (element_children($elements) as $child) {
      if (empty($child) || empty($elements[$child]) || is_numeric($child)) {
        continue;
      }
      // Process children.
      $this->cleanForm($elements[$child], $values, $parents);

      // Determine relative parents.
      $rel_parents = array_diff($elements[$child]['#parents'], $parents);
      $key_exists = NULL;
      $value = drupal_array_get_nested_value($values, $rel_parents, $key_exists);

      // Unset when applicable.
      if (!empty($elements[$child]['#markup'])) {
        static::drupal_array_remove_nested_value($values, $rel_parents);
      }
      elseif (
        $key_exists &&
        empty($value) &&
        !empty($elements[$child]['#fallback']) &&
        $value !== '0'
      ) {
        static::drupal_array_remove_nested_value($values, $rel_parents);
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
    // take the job into account.
    $defaults = $current_plugin->name == $original_plugin ? $job->settings : array();
    $defaults += $current_plugin->getDefaultSettings($job);

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
      '#weight' => -10,
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $form_state['values']['settings'][$plugin_type]['name'],
      '#title' => $static['title singular proper'],
      '#description' => t('Select which @plugin to use for this job.', array(
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
    if (
      $current_plugin->name == $original_plugin &&
      isset($job->settings[$plugin_type][$current_plugin->name]) &&
      is_array($job->settings[$plugin_type][$current_plugin->name])
    ) {
      $form_state['values']['settings'][$plugin_type][$current_plugin->name] += $job->settings[$plugin_type][$current_plugin->name];
    }
    $form_state['values']['settings'][$plugin_type][$current_plugin->name] += ultimate_cron_blank_values($defaults);

    $current_plugin->settingsForm($form, $form_state, $job);
    if (empty($form['settings'][$plugin_type][$current_plugin->name]['no_settings'])) {
      $current_plugin->fallbackalize(
        $form['settings'][$plugin_type][$current_plugin->name],
        $form_state['values']['settings'][$plugin_type][$current_plugin->name],
        $form_state['default_values']['settings'][$plugin_type][$current_plugin->name],
        FALSE
      );
      $form['settings'][$plugin_type][$current_plugin->name]['#description'] .= ' ' . $default_settings_link . '.';
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
    $elements = & $form['settings'][$plugin_type][$name];
    $values = & $form_state['values']['settings'][$plugin_type][$name];;
    $plugin->cleanForm($elements, $values, array(
      'settings',
      $plugin_type,
      $name
    ));
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
      $element = & $elements[$child];
      if (empty($element['#tree'])) {
        $param_values = & $values;
        $param_defaults = & $defaults;
      }
      else {
        $param_values = & $values[$child];
        $param_defaults = & $defaults[$child];
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
            $element['#description'] .= ' ' . t('(Blank = @default).', array('@default' => $this->settingsLabel($child, $defaults[$child])));
          }
          unset($element['#required']);
        }
      }
      elseif (!empty($element['#type']) && $remove_non_fallbacks) {
        unset($elements[$child]);
      }
      elseif (!isset($element['#default_value']) || $element['#default_value'] === '') {
        $empty = $element['#type'] == 'checkbox' ? FALSE : '';
        $values[$child] = !empty($defaults[$child]) ? $defaults[$child] : $empty;
        $element['#default_value'] = $values[$child];
      }
    }
  }
}
