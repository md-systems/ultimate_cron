<?php
/**
 * @file
 * Cache logger for Ultimate Cron.
 */

class UltimateCronCacheLogger extends UltimateCronLogger {
  public $log_entry_class = 'UltimateCronCacheLogEntry';

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'bin' => 'cache',
      'timeout' => 0,
    );
  }

  /**
   * Cleanup logs.
   */
  public function cleanup() {
  }


  /**
   * Load log entry.
   */
  public function load($job, $lock_id = NULL) {
    $log_entry = new $this->log_entry_class($this, $job);

    $settings = $job->getSettings('logger');

    if (!$lock_id) {
      $cache = cache_get('uc-name:' . $job->name, $settings['bin']);
      if (empty($cache) || empty($cache->data)) {
        return $log_entry;
      }
      $lock_id = $cache->data;
    }
    $cache = cache_get('uc-lid:' . $lock_id, $settings['bin']);

    if (!empty($cache->data)) {
      $log_entry->setData((array) $cache->data);
      $log_entry->finished = TRUE;
    }
    return $log_entry;
  }

  /**
   * Get log entries.
   */
  public function getLogEntries($job) {
    $log_entry = $this->load($job);
    return $log_entry->lid ? array($log_entry) : array();
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['bin'] = array(
      '#type' => 'textfield',
      '#title' => t('Cache bin'),
      '#description' => t('Select which cache bin to use for storing logs.'),
      '#default_value' => $values['bin'],
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $elements['timeout'] = array(
      '#type' => 'textfield',
      '#title' => t('Cache timeout'),
      '#description' => t('Seconds before cache entry expires (0 = never, -1 = on next general cache wipe.'),
      '#default_value' => $values['timeout'],
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }
}


class UltimateCronCacheLogEntry extends UltimateCronLogEntry {
  /**
   * Save log entry.
   */
  public function save() {
    if (!$this->lid) {
      return;
    }

    $settings = $this->job ? $this->job->getSettings('logger') : $this->logger->getDefaultSettings();
    $expire = $settings['timeout'] > 0 ? time() + $settings['timeout'] : $settings['timeout'];
    cache_set('uc-name:' . $this->job->name, $this->lid, $settings['bin'], $expire);
    cache_set('uc-lid:' . $this->lid, $this->getData(), $settings['bin'], $expire);
  }
}
