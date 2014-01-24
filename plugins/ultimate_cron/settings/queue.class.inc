<?php
/**
 * @file
 * Queue settings for Ultimate Cron.
 */

/**
 * Queue settings plugin class.
 */
class UltimateCronQueueSettings extends UltimateCronTaggedSettings {
  /**
   * Implements hook_cron_alter().
   */
  public function cron_alter(&$jobs) {
    $new_jobs = array();
    foreach ($jobs as $job) {
      $settings = $job->getSettings('settings');
      if (isset($settings['queue']['name'])) {
        if ($settings['queue']['throttle']) {
          for ($i = 2; $i <= $settings['queue']['threads']; $i++) {
            $name = $job->name . '_' . $i;
            $hook = $job->hook;
            $hook['settings']['queue']['master'] = $job->name;
            $hook['name'] = $name;
            $hook['title'] .= " (#$i)";
            $hook['immutable'] = TRUE;
            $new_jobs[$name] = ultimate_cron_prepare_job($name, $hook);
            $new_jobs[$name]->settings += $settings;
          }
        }
      }
    }
    $jobs += $new_jobs;
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'lease_time' => 30,
      'empty_delay' => 1,
      'item_delay' => 0,
      'throttle' => FALSE,
      'threads' => 4,
      'threshold' => 10,
      'time' => 15,
    );
  }

  /**
   * Settings form.
   */
  public function settingsForm(&$form, &$form_state) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['timeouts'] = array(
      '#type' => 'fieldset',
      '#title' => t('Timeouts'),
    );
    $elements['timeouts']['lease_time'] = array(
      '#title' => t("Queue lease time"),
      '#type' => 'textfield',
      '#default_value' => $values['lease_time'],
      '#description' => t('Seconds to claim a cron queue item.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $elements['timeouts']['time'] = array(
      '#title' => t('Time'),
      '#type' => 'textfield',
      '#default_value' => $values['time'],
      '#description' => t('Time in seconds to process items during a cron run.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $elements['delays'] = array(
      '#type' => 'fieldset',
      '#title' => t('Delays'),
    );
    $elements['delays']['empty_delay'] = array(
      '#title' => t("Empty delay"),
      '#type' => 'textfield',
      '#default_value' => $values['empty_delay'],
      '#description' => t('Seconds to delay processing of queue if queue is empty.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $elements['delays']['item_delay'] = array(
      '#title' => t("Item delay"),
      '#type' => 'textfield',
      '#default_value' => $values['item_delay'],
      '#description' => t('Seconds to wait between processing each item in a queue.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    $elements['throttle'] = array(
      '#title' => t('Throttle'),
      '#type' => 'checkbox',
      '#default_value' => $values['throttle'],
      '#description' => t('Throttle queues using multiple threads.'),
    );
    $elements['throttling'] = array(
      '#type' => 'fieldset',
      '#title' => t('Throttling'),
      '#states' => array(
        'visible' => array(':input[name="settings[' . $this->type . '][' . $this->name . '][throttle]"]' => array('checked' => TRUE))
      ),
    );
    $elements['throttling']['threads'] = array(
      '#title' => t('Threads'),
      '#type' => 'textfield',
      '#default_value' => $values['threads'],
      '#description' => t('Number of threads to use for queues.'),
      '#states' => array(
        'visible' => array(':input[name="settings[' . $this->type . '][' . $this->name . '][throttle]"]' => array('checked' => TRUE))
      ),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $elements['throttling']['threshold'] = array(
      '#title' => t('Threshold'),
      '#type' => 'textfield',
      '#default_value' => $values['threshold'],
      '#description' => t('Number of items in queue required to activate the next cron job.'),
      '#states' => array(
        'visible' => array(':input[name="settings[' . $this->type . '][' . $this->name . '][throttle]"]' => array('checked' => TRUE))
      ),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
  }

  /**
   * Throttle queues.
   *
   * Enables or disables queue threads depending on remaining items in queue.
   */
  public function throttle($job) {
    $settings = $job->getSettings('settings');
    if (!empty($settings['queue']['master'])) {
      // We always base the threads on the master.
      $job = ultimate_cron_job_load($settings['queue']['master']);
      $settings = $job->getSettings('settings');
    }
    if ($settings['queue']['throttle']) {
      $queue = DrupalQueue::get($settings['queue']['name']);
      $items = $queue->numberOfItems();
      for ($i = 2; $i <= $settings['queue']['threads']; $i++) {
        $name = $job->name . '_' . $i;
        $status = !empty($job->disabled) || ($items > ($i - 1) * $settings['queue']['threshold']);
        ultimate_cron_job_set_status($name, !$status);
      }
    }
  }
}
