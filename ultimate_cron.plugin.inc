<?php

class UltimateCronPlugin {
  public $name = '';
  public $title = '';
  public $description = '';
  public $plugin;
  public $settings = array();

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
    $this->key = 'ultimate_cron_plugin_' . $plugin['plugin type'] . '_' . $name . '_settings';
    $this->settings = variable_get($this->key, array());
  }

  /**
   * Get default settings.
   */
  public function getDefaultSettings($job = NULL) {
    $type = $this->plugin['plugin type'];
    if ($job && $job->hook[$type] == $this->name) {
      $hook_settings = $job->hook[$type . '_settings'];
    }
    else {
      $hook_settings = array();
    }
    return $this->settings + $hook_settings + $this->defaultSettings();
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array();
  }

  public function settingsFormValue($name, $value) {
    return $value;
  }

  public function settingsLabel($name, $value) {
    return $value;
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state) {
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
      $this->fallbackalize($element, $defaults, $remove_non_fallbacks);

      if (!empty($element['#fallback'])) {
        if (!$remove_non_fallbacks) {
          if (!empty($element['#use_fallback_value'])) {
            $element['#default_value'] = $this->settingsFormValue($child, $defaults[$child]);
          }
          else {
            if ($element['#type'] == 'select' && empty($element['#multiple'])) {
              $element['#options'] = array(
                '0' => t('Default (@default)', array('@default' => $this->settingsLabel($child, $defaults[$child]))),
              ) + $element['#options'];
            }
            else {
              $element['#description'] .= ' ' . t('(Blank = @default)', array('@default' => $this->settingsLabel($child, $defaults[$child])));
            }
          }
        }
        elseif (!empty($element['#fallback_required'])) {
          $element['#required'] = TRUE;
        }
      }
      elseif ($remove_non_fallbacks) {
        unset($elements[$child]);
      }
    }
  }
}

abstract class UltimateCronScheduler extends UltimateCronPlugin {
  abstract public function schedule($job);
}

abstract class UltimateCronLauncher extends UltimateCronPlugin {
  abstract public function launch($job);
}
