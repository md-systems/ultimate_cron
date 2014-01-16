<?php

class UltimateCronCacheLogger extends UltimateCronLogger {
  function defaultSettings() {
    return array(
      'bin' => 'cache',
      'timeout' => 0,
    );
  }

  function cleanup($job) {
  }

  function save() {
    $job = $this->job;
    $settings = $job->getSettings('logger');

    $expire = $settings['timeout'] > 0 ? time() + $settings['timeout'] : $settings['timeout'];
    cache_set('uc-name:' . $job->name, $this->log_entry->lid, $settings['bin']);
    cache_set('uc-lid:' . $this->log_entry->lid, $this->log_entry, $settings['bin']);
  }

  function load($job, $lid) {
    $settings = $job->getSettings('logger');

    $cache = $lid ? cache_get('uc-lid:' . $lid, $settings['bin']) : NULL;

    if (!empty($cache->data)) {
      $this->log_entry = $cache->data;
    }
    else {
      $this->log_entry->lid = NULL;
      $this->log_entry->start_time = 0;
      $this->log_entry->end_time = 0;
      $this->log_entry->init_message = '';
      $this->log_entry->message = '';
      $this->log_entry->severity = -1;
    }
    $this->log_entry->module = $job->hook['module'];
    $this->job = $job;
    return $this;
  }

  function loadLatest($job) {
    $settings = $job->getSettings('logger');

    $log_entry = NULL;
    $cache = cache_get('uc-name:' . $job->name, $settings['bin']);
    return $this->load($job, empty($cache->data) ? NULL : $cache->data);
  }

  function getLogEntries($job) {
    $log = $this->loadLatest($job);
    return $log->log_entry->lid ? array($log->log_entry) : array();
  }

  function settingsForm(&$form, &$form_state) {
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
