<?php

/**
 * @file
 * Contains \Drupal\ultimate_cron\CronJobListBuilder.
 */

namespace Drupal\ultimate_cron;

use Drupal\Component\Render\FormattableMarkup;
use Drupal\Core\Config\Entity\ConfigEntityListBuilder;
use Drupal\Core\Entity\EntityInterface;

/**
 * Defines a class to build a listing of cron jobs.
 *
 * @see \Drupal\ultimate_cron\Entity\CronJob
 */
class CronJobListBuilder extends ConfigEntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader() {
    $header = array();
    $header['module'] = array('data' => t('Module'));
    $header['title'] = array('data' => t('Title'));
    $header['scheduled'] = array('data' => t('Scheduled'));
    $header['started'] = array('data' => t('Last Run'));
    $header['duration'] = array('data' => t('Duration'));
    $header['status'] = array('data' => t('Status'));
    return $header + parent::buildHeader();
  }
  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity) {
    /* @var \Drupal\ultimate_cron\CronJobInterface $entity */
    $icon = drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass.png';
    $behind_icon = ['#prefix' => ' ', '#theme' => 'image', '#uri' => file_create_url($icon), '#title' => t('Job is behind schedule!')];

    $log_entry = $entity->loadLatestLogEntry();
    $row['module'] = array(
      'data' => $entity->getModuleName(),
      'title' => strip_tags($entity->getModuleDescription()),
    );
    $row['title'] = $entity->label();
    $row['scheduled']['data']['label']['#markup'] = $entity->getPlugin('scheduler')->formatLabel($entity);
    if ($entity->isScheduled()) {
      $row['scheduled']['data']['behind'] = $behind_icon;
    }
    else {
      $row['scheduled'] = $entity->getPlugin('scheduler')->formatLabel($entity);
    }
    // If the start time is 0, the jobs have never been run.
    $row['started'] = $log_entry->start_time ? \Drupal::service('date.formatter')->format($log_entry->start_time, "short") : $this->t('Never');

    // Display duration
    $progress = $entity->isLocked() ? $entity->formatProgress() : '';
    $row['duration'] = array(
      'data' => ['#markup' => '<span class="duration-time" data-src="' . $log_entry->getDuration() . '">' . $log_entry->formatDuration() . '</span> <span class="duration-progress">' . $progress . '</span>'],
      'class' => array('ctools-export-ui-duration'),
      'title' => strip_tags($log_entry->formatEndTime()),
    );

    if (!$entity->isValid()) {
      $row['status'] = $this->t('Missing');
    }
    elseif (!$entity->status()) {
      $row['status'] = $this->t('Disabled');
    }
    else {
      // Get the status from the launcher when running, otherwise use the last
      // log entry.
      if ($entity->isLocked() && $log_entry->lid == $entity->isLocked()) {
        list($status, $title) = $entity->getPlugin('launcher')->formatRunning($entity);
      }
      elseif ($log_entry->start_time && !$log_entry->end_time) {
        list($status, $title) = $entity->getPlugin('launcher')->formatUnfinished($entity);
      }
      else {
        list($status, $title) = $log_entry->formatSeverity();
        $title = $log_entry->message ? $log_entry->message : $title;
      }

      $row['status'] = [
        'data' => $status,
        'class' => array('ctools-export-ui-status'),
        'title' => strip_tags($title),
      ];
    }

    return $row + parent::buildRow($entity);
  }

  /**
   * {@inheritdoc}
   */
  public function getDefaultOperations(EntityInterface $entity) {
    $operations = parent::getDefaultOperations($entity);
    if ($entity->status() && $entity->isValid()) {
      $operations += [
        'run' => [
          'title' => t('Run'),
          'weight' => 9,
          'url' => $entity->toUrl('run'),
        ]
      ];
    }

    $operations += [
      'logs' => [
        'title' => t('Logs'),
        'weight' => 10,
        'url' => $entity->toUrl('logs'),
      ],
    ];

    // Invalid jobs can not be enabled nor disabled.
    if (!$entity->isValid()) {
      unset($operations['disable']);
      unset($operations['enable']);
    }

    return $operations;
  }

}
