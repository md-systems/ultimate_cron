<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\CronJobInterface.
 */

namespace Drupal\ultimate_cron;

use Drupal\Core\Config\Entity\ConfigEntityInterface;

interface CronJobInterface extends ConfigEntityInterface {

  /**
   * Gets the title of the created cron job.
   *
   * @return mixed
   *  Cron job title.
   */
  public function getTitle();

  /**
   * Gets the cron job callback string.
   *
   * @return string
   *  Callback string.
   */
  public function getCallback();

  /**
   * Gets the cron job module name used for the callback string.
   *
   * @return string
   *  Module name.
   */
  public function getModule();

  /**
   * Gets scheduler array which holds info about the scheduler settings.
   *
   * @return array
   *  Scheduler settings
   */
  public function getSchedulerId();

  /**
   * Gets launcher array which holds info about the launcher settings.
   *
   * @return array
   *  Launcher settings
   */
  public function getLauncherId();

  /**
   * Gets logger array which holds info about the logger settings.
   *
   * @return array
   *  Logger settings
   */
  public function getLoggerId();

  /**
   * Sets the title of the created cron job.
   *
   * @param $title
   * @return mixed
   *  Cron job title.
   */
  public function setTitle($title);

  /**
   * Sets the cron job callback string.
   *
   * @param $callback
   * @return string
   *  Callback string.
   */
  public function setCallback($callback);

  /**
   * Sets the cron job module name used for the callback string.
   *
   * @param $module
   * @return string
   *  Module name.
   */
  public function setModule($module);

  /**
   * Sets scheduler array which holds info about the scheduler settings.
   *
   * @param $scheduler_id
   * @return array
   *  Scheduler settings
   */
  public function setSchedulerId($scheduler_id);

  /**
   * Sets launcher array which holds info about the launcher settings.
   *
   * @param $launcher_id
   * @return array
   *  Launcher settings
   */
  public function setLauncherId($launcher_id);

  /**
   * Sets logger array which holds info about the logger settings.
   *
   * @param $logger_id
   * @return array
   *  Logger settings
   */
  public function setLoggerId($logger_id);

  /**
   * Check if the cron job is callable.
   *
   * @return bool
   *   TRUE if the job is callable, FALSE otherwise.
   */
  public function isValid();

}
