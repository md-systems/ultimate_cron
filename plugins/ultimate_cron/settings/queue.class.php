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
   * Get cron queues and static cache them.
   *
   * Works like module_invoke_all('cron_queue_info'), but adds
   * a 'module' to each item.
   *
   * @return array
   *   Cron queue definitions.
   */
  private function get_queues() {
    static $queues = NULL;
    if (!isset($queues)) {
      $queues = array();
      foreach (module_implements('cron_queue_info') as $module) {
        $items = module_invoke($module, 'cron_queue_info');
        if (is_array($items)) {
          foreach ($items as &$item) {
            $item['module'] = $module;
          }
          $queues += $items;
        }
      }
      drupal_alter('cron_queue_info', $queues);
    }
    return $queues;
  }

  /**
   * Implements hook_cronapi().
   */
  public function cronapi() {
    $items = array();

    // Grab the defined cron queues.
    $queues = self::get_queues();

    foreach ($queues as $name => $info) {
      if (!empty($info['skip on cron'])) {
        continue;
      }

      $items['queue_' . $name] = array(
        'title' => t('Queue: !name', array('!name' => $name)),
        'callback' => array(get_class($this), 'worker_callback'),
        'scheduler' => array(
          'simple' => array(
            'rules' => array('* * * * *'),
          ),
          'crontab' => array(
            'rules' => array('* * * * *'),
          ),
        ),
        'settings' => array(
          'queue' => array(
            'name' => $name,
            'worker callback' => $info['worker callback'],
          ),
        ),
        'tags' => array('queue', 'core'),
        'module' => $info['module'],
      );
      if (isset($info['time'])) {
        $items['queue_' . $name]['settings']['queue']['time'] = $info['time'];
      }
    }

    return $items;
  }

  /**
   * Process a cron queue.
   *
   * This is a wrapper around the cron queues "worker callback".
   *
   * @param UltimateCronJob $job
   *   The job being run.
   */
  static public function worker_callback($job) {
    $settings = $job->getPluginSettings('settings');
    $queue = DrupalQueue::get($settings['queue']['name']);
    $function = $settings['queue']['worker callback'];

    // Re-throttle.
    $job->getPlugin('settings', 'queue')->throttle($job);

    $end = microtime(TRUE) + $settings['queue']['time'];
    $items = 0;
    while (microtime(TRUE) < $end) {
      $item = $queue->claimItem($settings['queue']['lease_time']);
      if (!$item) {
        if ($settings['queue']['empty_delay']) {
          usleep($settings['queue']['empty_delay'] * 1000000);
          continue;
        }
        else {
          break;
        }
      }
      try {
        if ($settings['queue']['item_delay']) {
          if ($items == 0) {
            // Move the boundary if using a throttle, to avoid waiting for nothing.
            $end -= $settings['queue']['item_delay'] * 1000000;
          }
          else {
            // Sleep before retrieving.
            usleep($settings['queue']['item_delay'] * 1000000);
          }
        }
        $function($item->data);
        $queue->deleteItem($item);
        $items++;
      }
      catch (Exception $e) {
        // Just continue ...
        watchdog($job->hook['module'], "Queue item @item_id from queue @queue failed with message @message", array(
          '@item_id' => $item->item_id,
          '@queue' => $settings['queue']['name'],
          '@message' => $e->getMessage()
        ), WATCHDOG_ERROR);
      }
    }
    watchdog($job->hook['module'], 'Processed @items items from queue @queue', array(
      '@items' => $items,
      '@queue' => $settings['queue']['name'],
    ), WATCHDOG_INFO);

    return;
  }

  /**
   * Implements hook_cron_alter().
   */
  public function cron_alter(&$jobs) {
    $new_jobs = array();
    foreach ($jobs as $job) {
      if (!$this->isValid($job)) {
        continue;
      }
      $settings = $job->getSettings();
      if (isset($settings['settings']['queue']['name'])) {
        if ($settings['settings']['queue']['throttle']) {
          for ($i = 2; $i <= $settings['settings']['queue']['threads']; $i++) {
            $name = $job->name . '_' . $i;
            $hook = $job->hook;
            $hook['settings']['queue']['master'] = $job->name;
            $hook['name'] = $name;
            $hook['title'] .= " (#$i)";
            $hook['immutable'] = TRUE;
            $new_jobs[$name] = ultimate_cron_prepare_job($name, $hook);
            $new_jobs[$name]->settings = $settings + $new_jobs[$name]->settings;
          }
        }
      }
    }
    $jobs += $new_jobs;
  }

  /**
   * Implements hook_cron_alter().
   */
  public function cron_pre_schedule($job) {
    $settings = $job->getSettings('settings');
    static $throttled = FALSE;
    if (!$throttled && !empty($job->hook['settings']['queue']['master'])) {
      $throttled = TRUE;
      $this->throttle($job);
    }
  }

  /**
   * Default settings.
   */
  public function defaultSettings() {
    return array(
      'lease_time' => 30,
      'empty_delay' => 0,
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
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $elements = &$form['settings'][$this->type][$this->name];
    $values = &$form_state['values']['settings'][$this->type][$this->name];

    $elements['timeouts'] = array(
      '#type' => 'fieldset',
      '#title' => t('Timeouts'),
    );
    $elements['timeouts']['lease_time'] = array(
      '#parents' => array('settings', $this->type, $this->name, 'lease_time'),
      '#title' => t("Queue lease time"),
      '#type' => 'textfield',
      '#default_value' => $values['lease_time'],
      '#description' => t('Seconds to claim a cron queue item.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $elements['timeouts']['time'] = array(
      '#parents' => array('settings', $this->type, $this->name, 'time'),
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
      '#parents' => array('settings', $this->type, $this->name, 'empty_delay'),
      '#title' => t("Empty delay"),
      '#type' => 'textfield',
      '#default_value' => $values['empty_delay'],
      '#description' => t('Seconds to delay processing of queue if queue is empty (0 = end job).'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );
    $elements['delays']['item_delay'] = array(
      '#parents' => array('settings', $this->type, $this->name, 'item_delay'),
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
      '#parents' => array('settings', $this->type, $this->name, 'threads'),
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
      '#parents' => array('settings', $this->type, $this->name, 'threshold'),
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
