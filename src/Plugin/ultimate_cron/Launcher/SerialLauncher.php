<?php
/**
 * @file
 * Serial cron job launcher for Ultimate Cron.
 */
namespace Drupal\ultimate_cron\Plugin\ultimate_cron\Launcher;

use Drupal\ultimate_cron\Launcher\LauncherBase;
use Exception;

/**
 * Ultimate Cron launcher plugin class.
 *
 * @LauncherPlugin(
 *   id = "serial",
 *   title = @Translation("Serial"),
 *   description = @Translation("Launches scheduled jobs in the same thread and runs them consecutively."),
 * )
 */
class SerialLauncher extends LauncherBase {

  public $currentThread = NULL;

  /**
   * Implements hook_cron_alter().
   */
  public function cron_alter(&$jobs) {
    $lock = \Drupal::service('ultimate_cron.lock');
    if (!empty($lock->{$killable})) {
      $jobs['ultimate_cron_plugin_launcher_serial_cleanup']->hook['tags'][] = 'killable';
    }
  }

  /**
   * Default settings.
   */
  public function defaultConfiguration() {
    return array(
      'timeouts' => array(
        'lock_timeout' => 3600,
        'max_execution_time' => 3600,
      ),
      'launcher' => array(
        'max_threads' => 1,
        'thread' => 'any',
        'poorman_keepalive' => FALSE,
      ),
    ) + parent::defaultConfiguration();
  }

  /**
   * Settings form for the crontab scheduler.
   */
  public function settingsForm(&$form, &$form_state, $job = NULL) {
    $form['timeouts'] = array(
      '#type' => 'fieldset',
      '#title' => t('Timeouts'),
    );

    $form['launcher'] = array(
      '#type' => 'fieldset',
      '#title' => t('Launching options'),
    );

    $form['timeouts']['lock_timeout'] = array(
      '#title' => t("Job lock timeout"),
      '#type' => 'textfield',
      '#default_value' => $this->configuration['timeouts']['lock_timeout'],
      '#description' => t('Number of seconds to keep lock on job.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
    );

    if (!$job) {
      $form['timeouts']['max_execution_time'] = array(
        '#title' => t("Maximum execution time"),
        '#type' => 'textfield',
        '#default_value' => $this->configuration['timeouts']['max_execution_time'],
        '#description' => t('Maximum execution time for a cron run in seconds.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
      );
      $form['launcher']['max_threads'] = array(
        '#title' => t("Maximum number of launcher threads"),
        '#type' => 'number',
        '#default_value' => $this->configuration['launcher']['max_threads'],
        '#description' => t('The maximum number of launch threads that can be running at any given time.'),
        '#fallback' => TRUE,
        '#required' => TRUE,
        '#weight' => 1,
      );
      $form['launcher']['poorman_keepalive'] = array(
        '#title' => t("Poormans cron keepalive"),
        '#type' => 'checkbox',
        '#default_value' => $this->configuration['launcher']['poorman_keepalive'],
        '#description' => t('Retrigger poormans cron after it has finished. Requires $base_url to be accessible from the webserver.'),
        '#fallback' => TRUE,
        '#weight' => 3,
      );

      return $form;
    }
    else {
      $settings = $this->getConfiguration();
      $max_threads = $settings['max_threads'];
    }

    $options = array(
      'any' => t('-- Any -- '),
      'fixed' => t('-- Fixed -- '),
    );
    for ($i = 1; $i <= $max_threads; $i++) {
      $options[$i] = $i;
    }


    $elements['launcher']['thread'] = array(
      '#parents' => array('settings', $this->type, $this->name, 'thread'),
      '#title' => t("Run in thread"),
      '#type' => 'select',
      '#default_value' => $values['thread'],
      '#options' => $options,
      '#description' => t('Which thread to run in when invoking with ?thread=N. Note: This setting only has an effect when cron is run through cron.php with an argument ?thread=N or through Drush with --options=thread=N.'),
      '#fallback' => TRUE,
      '#required' => TRUE,
      '#weight' => 2,
    );
  }

  /**
   * Settings form validator.
   */
  public function settingsFormValidate(&$form, &$form_state, $job = NULL) {
    $elements = & $form['settings'][$this->type][$this->name];
    $values = & $form_state['values']['settings'][$this->type][$this->name];
    if (!$job) {
      if (intval($values['max_threads']) <= 0) {
        form_set_error("settings[$this->type][$this->name", t('%title must be greater than 0', array(
          '%title' => $elements['launcher']['max_threads']['#title']
        )));
      }
    }
  }

  /**
   * Lock job.
   */
  public function lock($job) {
    $settings = $job->getSettings('launcher')['serial'];
    $timeout = $settings['lock_timeout'];

    $lock = \Drupal::service('ultimate_cron.lock');
    if ($lock_id = $lock->lock($job->id(), $timeout)) {
      $lock_id = $this->getPluginId() . '-' . $lock_id;
      return $lock_id;
    }
    return FALSE;
  }

  /**
   * Unlock job.
   */
  public function unlock($lock_id, $manual = FALSE) {
    list($launcher, $lock_id) = explode('-', $lock_id, 2);
    $lock = \Drupal::service('ultimate_cron.lock');
    return $lock->unlock($lock_id);
  }

  /**
   * Check if job is locked.
   */
  public function isLocked($job) {
    $lock = \Drupal::service('ultimate_cron.lock');
    $lock_id = $lock->isLocked($job->id());
    return $lock_id ? $this->name . '-' . $lock_id : $lock_id;
  }

  /**
   * Check lock for multiple jobs.
   */
  public function isLockedMultiple($jobs) {
    $names = array();
    foreach ($jobs as $job) {
      $names[] = $job->id();
    }
    $lock = \Drupal::service('ultimate_cron.lock');
    $lock_ids = $lock->isLockedMultiple($names);
    foreach ($lock_ids as &$lock_id) {
      $lock_id = $lock_id ? $this->name . '-' . $lock_id : $lock_id;
    }
    return $lock_ids;
  }

  /**
   * Cleanup.
   */
  public function cleanup() {
    $lock = \Drupal::service('ultimate_cron.lock');
    $lock->cleanup();
  }

  /**
   * Launcher.
   */
  public function launch($job) {
    $lock_id = $job->lock();

    if (!$lock_id) {
      return FALSE;
    }

    if ($this->currentThread) {
      $init_message = t('Launched in thread @current_thread', array(
        '@current_thread' => $this->currentThread,
      ));
    }
    else {
      $init_message = t('Launched manually');
    }
    $log_entry = $job->startLog($lock_id, $init_message);

    drupal_set_message(t('@name: @init_message', array(
      '@name' => $job->id(),
      '@init_message' => $init_message,
    )));

    // Run job.
    try {
      $job->run();
    } catch (Exception $e) {
      watchdog('serial_launcher', 'Error executing %job: @error', array(
        '%job' => $job->id(),
        '@error' => $e->getMessage()
      ), WATCHDOG_ERROR);
      $log_entry->finish();
      $job->unlock($lock_id);
      return FALSE;
    }

    $log_entry->finish();
    $job->unlock($lock_id);
    return TRUE;
  }

  /**
   * Find a free thread for running cron jobs.
   */
  public function findFreeThread($lock, $lock_timeout = NULL, $timeout = 3) {
    $settings = $this->getConfiguration();

    // Find a free thread, try for 3 seconds.
    $delay = $timeout * 1000000;
    $sleep = 25000;

    $lock_service = \Drupal::service('ultimate_cron.lock');
    do {
      for ($thread = 1; $thread <= $settings['max_threads']; $thread++) {
        if ($thread != $this->currentThread) {
          $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
          if (!$lock_service->isLocked($lock_name)) {
            if ($lock) {
              if ($lock_id = $lock_service->lock($lock_name, $lock_timeout)) {
                return array($thread, $lock_id);
              }
            }
            else {
              return array($thread, FALSE);
            }
          }
        }
      }
      if ($delay > 0) {
        usleep($sleep);
        // After each sleep, increase the value of $sleep until it reaches
        // 500ms, to reduce the potential for a lock stampede.
        $delay = $delay - $sleep;
        $sleep = min(500000, $sleep + 25000, $delay);
      }
    } while ($delay > 0);
    return array(FALSE, FALSE);
  }

  /**
   * Launch manager.
   */
  public function launchJobs($jobs) {
    $lock = \Drupal::service('ultimate_cron.lock');
    $settings = $this->getConfiguration();

    // Set proper max execution time.
    $max_execution_time = ini_get('max_execution_time');
    $lock_timeout = max($max_execution_time, $settings['max_execution_time']);

    // We only lock for 55 seconds at a time, to give room for other cron
    // runs.
    $lock_timeout = 55;

    if (!empty($_GET['thread'])) {
      self::setGlobalOption('thread', $_GET['thread']);
    }

    if ($thread = intval(self::getGlobalOption('thread'))) {
      if ($thread < 1 || $thread > $settings['max_threads']) {
        watchdog('serial_launcher', "Invalid thread available for starting launch thread", array(), WATCHDOG_WARNING);
        return;
      }

      $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
      $lock_id = NULL;
      if (!$lock->isLocked($lock_name)) {
        $lock_id = $lock->lock($lock_name, $lock_timeout);
      }
      if (!$lock_id) {
        watchdog('serial_launcher', "Thread @thread is already running", array(
          '@thread' => $thread,
        ), WATCHDOG_WARNING);
      }
    }
    else {
      $timeout = 1;
      list($thread, $lock_id) = $this->findFreeThread(TRUE, $lock_timeout, $timeout);
    }
    $this->currentThread = $thread;

    if (!$thread) {
      watchdog('serial_launcher', "No free threads available for launching jobs", array(), WATCHDOG_WARNING);
      return;
    }

    if ($max_execution_time && $max_execution_time < $settings['max_execution_time']) {
      set_time_limit($settings['max_execution_time']);
    }

    watchdog('serial_launcher', "Cron thread %thread started", array('%thread' => $thread), WATCHDOG_INFO);

    $this->runThread($lock_id, $thread, $jobs);
    $lock->unlock($lock_id);
  }

  /**
   * Run jobs in thread.
   *
   * @param string $lock_id
   *   The lock id.
   * @param string $thread
   *   The tread number.
   * @param array $jobs
   *   The UltimateCronJobs to run.
   */
  public function runThread($lock_id, $thread, $jobs) {
    $lock = \Drupal::service('ultimate_cron.lock');
    $lock_name = 'ultimate_cron_serial_launcher_' . $thread;
    foreach ($jobs as $job) {
      $settings = $job->settings;
      switch ($settings['thread']) {
        case 'any':
          $settings['thread'] = $thread;
          break;

        case 'fixed':
          $settings['thread'] = ($job->getUniqueID() % $settings['max_threads']) + 1;
          break;
      }
      if ((!self::getGlobalOption('thread') || $settings['thread'] == $thread) && $job->isScheduled()) {
        $job->launch();
        // Be friendly, and check if someone else has taken the lock.
        // If they have, bail out, since someone else is now handling
        // this thread.
        if ($current_lock_id = $lock->isLocked($lock_name)) {
          if ($current_lock_id !== $lock_id) {
            return;
          }
        }
        else {
          // If lock is free, then take the lock again.
          $lock_id = $lock->lock($lock_name);
          if (!$lock_id) {
            // Race-condition, someone beat us to it.
            return;
          }
        }
      }
    }
  }

  /**
   * Poormans cron launcher.
   */
  public function launchPoorman() {
    $lock = \Drupal::service('ultimate_cron.lock');
    $settings = $this->getConfiguration();
    // Is it time to run cron?
    $cron_last = variable_get('cron_last', 0);
    $cron_next = floor(($cron_last + 60) / 60) * 60;
    $time = time();
    if ($time < $cron_next) {
      if ($settings['poorman_keepalive'] && $lock_id = $lock->lock('ultimate_cron_poorman_serial', 60)) {
        ultimate_cron_poorman_page_flush();
        $sleep = $cron_next - $time;
        sleep($sleep);
        /*
        while ($sleep--) {
          error_log("SLEEPING3: $sleep");
          sleep(1);
        }
        /**/
        ultimate_cron_poorman_trigger();
        $lock->unLock($lock_id);
      }
      return;
    }

    unset($_GET['thread']);
    ultimate_cron_poorman_page_flush();
    ultimate_cron_run_launchers();

    // Check poorman settings. If launcher has changed, we don't want
    // to keepalive.
    $poorman = ultimate_cron_plugin_load('settings', 'poorman');
    if (!$poorman) {
      return;
    }

    $settings = $poorman->getConfiguration();
    if (!$settings['launcher'] || $settings['launcher'] !== $this->name) {
      return;
    }

    $settings = $this->getConfiguration();
    if ($settings['poorman_keepalive'] && $lock_id = $lock->lock('ultimate_cron_poorman_serial', 60)) {
      // Is it time to run cron? If not wait before re-launching.
      $cron_last = variable_get('cron_last', 0);
      $cron_next = floor(($cron_last + 60) / 60) * 60;
      $time = time();
      if ($time < $cron_next) {
        $sleep = $cron_next - $time;
        sleep($sleep);
        /*
        while ($sleep--) {
          error_log("SLEEPING4: $sleep");
          sleep(1);
        }
        /**/
      }

      $lock->unLock($lock_id);
      ultimate_cron_poorman_trigger();
    }
  }
}
