<?php
namespace Drupal\ultimate_cron;

use Drupal\ultimate_cron\Logger\LogEntry;

class CacheLogEntry extends LogEntry {
  /**
   * Save log entry.
   */
  public function save() {
    if (!$this->lid) {
      return;
    }

    if ($this->log_type != ULTIMATE_CRON_LOG_TYPE_NORMAL) {
      return;
    }

    $job = ultimate_cron_job_load($this->name);

    $settings = $job->getSettings('logger');

    $expire = $settings['timeout'] > 0 ? time() + $settings['timeout'] : $settings['timeout'];
    cache_set('uc-name:' . $this->name, $this->lid, $settings['bin'], $expire);
    cache_set('uc-lid:' . $this->lid, $this->getData(), $settings['bin'], $expire);
  }
}
