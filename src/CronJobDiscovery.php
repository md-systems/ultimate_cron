<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\CronJobDiscovery.
 *
 * Cron Job helper class.
 */
namespace Drupal\ultimate_cron;

use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\ultimate_cron\Entity\CronJob;

/**
 * Discovery and instantiation of default cron jobs.
 */
class CronJobDiscovery {

  /**
   * @var \Drupal\Core\Extension\ModuleHandlerInterface
   */
  protected $moduleHandler;

  /**
   * CronJobDiscovery constructor.
   *
   * @param \Drupal\Core\Extension\ModuleHandlerInterface $module_handler
   *   The module handler.
   */
  public function __construct(ModuleHandlerInterface $module_handler) {
    $this->moduleHandler = $module_handler;
  }

  /**
   * Automatically discovers and creates default cron jobs.
   */
  public function discoverCronJobs() {
    foreach ($this->getHooks() as $id => $info) {
      $this->ensureCronJobExists($info, $id);
    }
  }

  /**
   * Creates a new cron job with specific values.
   *
   * @param array $info
   *   Module info.
   * @param string $id
   *   Module name.
   */
  protected function ensureCronJobExists($info, $id) {
    $job = NULL;
    if (!CronJob::load($id)) {
      $values = array(
        'title' => $this->getJobTitle($id),
        'id' => $id,
        'module' => $info['module'],
        'callback' => $info['callback'],
      );

      $job = CronJob::create($values);

      $job->save();
    }
  }

  /**
   * Returns the job title for a given ID.
   *
   * @param string $id
   *   The default cron job ID.
   *
   * @return \Drupal\Core\StringTranslation\TranslatableMarkup
   *   The default job title.
   */
  protected function getJobTitle($id) {
    $titles = array();

    $titles['comment_cron'] = t('Store the maximum possible comments per thread');
    $titles['dblog_cron'] = t('Remove expired log messages and flood control events');
    $titles['field_cron'] = t('Purges deleted Field API data');
    $titles['file_cron'] = t('Deletes temporary files');
    $titles['history_cron'] = t('Deletes history');
    $titles['search_cron'] = t('Updates indexable active search pages');
    $titles['system_cron'] = t('Cleanup (caches, batch, flood, temp-files, etc.)');
    $titles['update_cron'] = t('Update indexes');
    $titles['node_cron'] = t('Mark old nodes as read');
    $titles['aggregator_cron'] = t('Refresh feeds');
    $titles['ultimate_cron_cron'] = t('Runs internal cleanup operations');
    $titles['statistics_cron'] = t('Reset counts and clean up');
    $titles['tracker_cron'] = t('Update tracker index');

    if (isset($titles[$id])) {
      return $titles[$id];
    }
    return t('Default cron handler');
  }

  /**
   * Get all cron hooks defined.
   *
   * @return array
   *   All hook definitions available.
   */
  protected function getHooks() {
    $hooks = array();
    // Generate list of jobs provided by modules.
    $modules = array_keys($this->moduleHandler->getModuleList());
    foreach ($modules as $module) {
      $hooks += $this->getModuleHooks($module);
    }

    return $hooks;
  }

  /**
   * Get cron hooks declared by a module.
   *
   * @param string $module
   *   Name of module.
   *
   * @return array
   *   Hook definitions for the specified module.
   */
  protected function getModuleHooks($module) {
    $items = array();

    // Add hook_cron() if applicable.
    if ($this->moduleHandler->implementsHook($module, 'cron')) {
      $info = system_get_info('module', $module);
      $callback = "{$module}_cron";
      $items[$callback] = array(
        'module' => $module,
        'title' =>  isset($titles[$callback]) ? $titles[$callback] : 'Default cron handler',
        'configure' => empty($info['configure']) ? NULL : $info['configure'],
        'callback' => $callback,
        'tags' => array(),
        'pass job argument' => FALSE,
      );
      $items["{$module}_cron"]['tags'][] = 'core';
    }

    return $items;
  }

}

