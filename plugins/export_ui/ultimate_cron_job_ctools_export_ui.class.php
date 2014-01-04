<?php
/**
 * @file
 * Export-ui handler for the Ultimate Cron jobs.
 */

class ultimate_cron_job_ctools_export_ui extends ctools_export_ui {
  function hook_menu(&$items) {
    parent::hook_menu($items);

    unset($items['admin/config/system/cron/jobs/add']);
    unset($items['admin/config/system/cron/jobs/import']);
    unset($items['admin/config/system/cron/jobs/list/%ctools_export_ui/delete']);
    unset($items['admin/config/system/cron/jobs/list/%ctools_export_ui/clone']);

    #var_dump($items['admin/config/system/cron']);
    #unset($items['admin/config/system/cron']);
    #return;
    #return;
  }
}
