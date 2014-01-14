<?php
/**
 * @file
 * Export-ui handler for the Ultimate Cron jobs.
 */

class ultimate_cron_job_ctools_export_ui extends ctools_export_ui {
  /**
   * Ensure we cannot add, import, delete or clone.
   */
  function hook_menu(&$items) {
    parent::hook_menu($items);

    unset($items['admin/config/system/cron/jobs/add']);
    unset($items['admin/config/system/cron/jobs/import']);
    unset($items['admin/config/system/cron/jobs/list/%ctools_export_ui/delete']);
    unset($items['admin/config/system/cron/jobs/list/%ctools_export_ui/clone']);
  }

  /**
   * Ensure that we cannot clone from the operations link list.
   */
  function build_operations($item) {
    $allowed_operations = parent::build_operations($item);
    unset($allowed_operations['clone']);
    return $allowed_operations;
  }

  /**
   * Create the filter/sort form at the top of a list of exports.
   *
   * This handles the very default conditions, and most lists are expected
   * to override this and call through to parent::list_form() in order to
   * get the base form and then modify it as necessary to add search
   * gadgets for custom fields.
   */
  function list_form(&$form, &$form_state) {
    parent::list_form($form, $form_state);

    $all = array('all' => t('- All -'));

    $form['top row']['status'] = array(
      '#type' => 'select',
      '#title' => t('Status'),
      '#options' => $all + watchdog_severity_levels(),
      '#default_value' => 'all',
      '#weight' => -2,
    );

    $jobs = ultimate_cron_get_hooks();
    $modules = array();
    foreach ($jobs as $job) {
      $info = ultimate_cron_get_module_info($job['module']);
      $modules[$job['module']] = $info && !empty($info['name']) ? $info['name'] : $job['module'];
    }

    $form['top row']['module'] = array(
      '#type' => 'select',
      '#title' => t('Module'),
      '#options' => $all + $modules,
      '#default_value' => 'all',
      '#weight' => -1,
    );
  }

  /**
   * Determine if a row should be filtered out.
   *
   * This handles the default filters for the export UI list form. If you
   * added additional filters in list_form() then this is where you should
   * handle them.
   *
   * @return bool
   *   TRUE if the item should be excluded.
   */
  function list_filter($form_state, $item) {
    $schema = ctools_export_get_schema($this->plugin['schema']);
    if ($form_state['values']['storage'] != 'all' && $form_state['values']['storage'] != $item->{$schema['export']['export type string']}) {
      return TRUE;
    }

    if ($form_state['values']['module'] != 'all' && $form_state['values']['module'] != $item->hook['module']) {
      return TRUE;
    }

    if ($form_state['values']['status'] != 'all' && $form_state['values']['status'] != $item->severity) {
      return TRUE;
    }

    if ($form_state['values']['disabled'] != 'all' && $form_state['values']['disabled'] != !empty($item->disabled)) {
      return TRUE;
    }

    if ($form_state['values']['search']) {
      $search = strtolower($form_state['values']['search']);
      foreach ($this->list_search_fields() as $field) {
        if (strpos(strtolower($item->$field), $search) !== FALSE) {
          $hit = TRUE;
          break;
        }
      }
      if (empty($hit)) {
        return TRUE;
      }
    }
  }

  /**
   * Provide the table header.
   *
   * If you've added columns via list_build_row() but are still using a
   * table, override this method to set up the table header.
   */
  function list_table_header() {
    $header = array();
    $header[] = array('data' => t('Module'), 'class' => array('ctools-export-ui-module'));
    if (!empty($this->plugin['export']['admin_title'])) {
      $header[] = array('data' => t('Title'), 'class' => array('ctools-export-ui-title'));
    }

    // $header[] = array('data' => t('Name'), 'class' => array('ctools-export-ui-name'));
    $header[] = array('data' => t('Scheduled'), 'class' => array('ctools-export-ui-scheduled'));
    $header[] = array('data' => t('Started'), 'class' => array('ctools-export-ui-last-start-time'));
    $header[] = array('data' => t('Duration'), 'class' => array('ctools-export-ui-duration'));
    $header[] = array('data' => t('Storage'), 'class' => array('ctools-export-ui-storage'));
    $header[] = array('data' => t('Status'), 'class' => array('ctools-export-ui-status'));
    $header[] = array('data' => t('Operations'), 'class' => array('ctools-export-ui-operations'));

    return $header;
  }

  /**
   * Provide a list of sort options.
   *
   * Override this if you wish to provide more or change how these work.
   * The actual handling of the sorting will happen in build_row().
   */
  function list_sort_options() {
    if (!empty($this->plugin['export']['admin_title'])) {
      $options = array(
        'disabled' => t('Enabled, module, title'),
        $this->plugin['export']['admin_title'] => t('Title'),
      );
    }
    else {
      $options = array(
        'disabled' => t('Enabled, module, name'),
      );
    }

    $options += array(
      'name' => t('Name'),
      'storage' => t('Storage'),
    );

    return $options;
  }

  /**
   * Build a row based on the item.
   *
   * By default all of the rows are placed into a table by the render
   * method, so this is building up a row suitable for theme('table').
   * This doesn't have to be true if you override both.
   */
  function list_build_row($item, &$form_state, $operations) {
    // Set up sorting.
    $name = $item->{$this->plugin['export']['key']};
    $schema = ctools_export_get_schema($this->plugin['schema']);

    // Note: $item->{$schema['export']['export type string']} should have already been set up by export.inc so
    // we can use it safely.
    switch ($form_state['values']['order']) {
      case 'disabled':
        $this->rows[$name]['sort'] = array(
          (int) !empty($item->disabled),
          $item->getModuleName(),
          empty($this->plugin['export']['admin_title']) ? $name : $item->{$this->plugin['export']['admin_title']}
        );
        break;

      case 'title':
        $this->rows[$name]['sort'] = $item->{$this->plugin['export']['admin_title']};
        break;

      case 'start_time':
        $this->rows[$name]['sort'] = $item->start_time;
        break;

      case 'storage':
        $this->sorts[$name] = $item->{$schema['export']['export type string']} . $name;
        break;
    }

    $this->rows[$name]['data'] = array();

    // Enabled/disabled.
    $this->rows[$name]['class'] = !empty($item->disabled) ? array('ctools-export-ui-disabled') : array('ctools-export-ui-enabled');

    // Module.
    $this->rows[$name]['data'][] = array(
      'data' => check_plain($item->getModuleName()),
      'class' => array('ctools-export-ui-module'),
      'title' => check_plain($item->getModuleDescription()),
    );

    // If we have an admin title, make it the first row.
    if (!empty($this->plugin['export']['admin_title'])) {
      $this->rows[$name]['data'][] = array(
        'data' => check_plain($item->{$this->plugin['export']['admin_title']}),
        'class' => array('ctools-export-ui-title'),
        'title' => $item->name,
      );
    }

    // Schedule settings.
    $label = $item->getPlugin('scheduler')->getScheduledLabel($item);
    $verbose = $item->getPlugin('scheduler')->getScheduledLabelVerbose($item);
    $this->rows[$name]['data'][] = array(
      'data' => $label,
      'class' => array('ctools-export-ui-scheduled'),
      'title' => $verbose,
    );

    // Started and duration.
    $log = $item->getPlugin('logger')->loadLatest($item);
    $start_time = $log->start_time ? format_date((int) $log->start_time, 'custom', 'Y-m-d H:i:s') : t('Never');
    $this->rows[$name]['data'][] = array('data' => $start_time, 'class' => array('ctools-export-ui-last-start-time'));

    $duration = NULL;
    if ($log->start_time && $log->end_time) {
      $duration = (int) ($log->end_time - $log->start_time);
    }
    elseif ($log->start_time) {
      $duration = (int) (microtime(TRUE) - $log->start_time);
    }

    switch (TRUE) {
      case $duration >= 86400:
        $format = 'd H:i:s';
        break;

      case $duration >= 3600:
        $format = 'H:i:s';
        break;

      default:
        $format = 'i:s';
    }
    $duration = isset($duration) ? gmdate($format, $duration) : t('N/A');
    $this->rows[$name]['data'][] = array(
      'data' => $duration,
      'class' => array('ctools-export-ui-duration'),
      'title' => $log->end_time ? t('Previous run finished @ @end_time', array(
        '@end_time' => format_date((int) $log->end_time, 'custom', 'Y-m-d H:i:s'),
      )) : '',
    );


    // Storage.
    $this->rows[$name]['data'][] = array('data' => check_plain($item->{$schema['export']['export type string']}), 'class' => array('ctools-export-ui-storage'));

    // Status.
    if ($log->start_time && !$log->end_time) {
      $file = drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass.png';
    }
    else {
      switch ($log->severity) {
        case WATCHDOG_EMERGENCY:
        case WATCHDOG_ALERT:
        case WATCHDOG_CRITICAL:
        case WATCHDOG_ERROR:
          $file = 'misc/message-16-error.png';
          break;
        case WATCHDOG_WARNING:
        case WATCHDOG_NOTICE:
          $file = 'misc/message-16-warning.png';
          break;
        case WATCHDOG_INFO:
        case WATCHDOG_DEBUG:
          $file = 'misc/message-16-info.png';
          break;

        default;
          $file = 'misc/message-16-ok.png';
      }
    }
    $image = theme('image', array(
      'path' => $file,
    ));
    $this->rows[$name]['data'][] = array(
      'data' => $image,
      'class' => array('ctools-export-ui-status'),
      'title' => $log->message ? $log->message : t('No info'),
    );

    // Operations.
    $ops = theme('links__ctools_dropbutton', array('links' => $operations, 'attributes' => array('class' => array('links', 'inline'))));

    $this->rows[$name]['data'][] = array('data' => $ops, 'class' => array('ctools-export-ui-operations'));

    // Add an automatic mouseover of the description if one exists.
    if (!empty($this->plugin['export']['admin_description'])) {
      $this->rows[$name]['title'] = $item->{$this->plugin['export']['admin_description']};
    }
  }

  /**
   * Submit the filter/sort form.
   *
   * This submit handler is actually responsible for building up all of the
   * rows that will later be rendered, since it is doing the filtering and
   * sorting.
   *
   * For the most part, you should not need to override this method, as the
   * fiddly bits call through to other functions.
   */
  function list_form_submit(&$form, &$form_state) {
    // Filter and re-sort the pages.
    $plugin = $this->plugin;

    $prefix = ctools_export_ui_plugin_base_path($plugin);

    foreach ($this->items as $name => $item) {
      // Call through to the filter and see if we're going to render this
      // row. If it returns TRUE, then this row is filtered out.
      if ($this->list_filter($form_state, $item)) {
        continue;
      }

      $operations = $this->build_operations($item);

      $this->list_build_row($item, $form_state, $operations);
    }

    // Now actually sort.
    uasort($this->rows, array(get_class($this), 'multi_column_sort'));

    if ($form_state['values']['sort'] == 'desc') {
      $this->rows = array_reverse($this->rows);
    }
    foreach ($this->rows as &$row) {
      unset($row['sort']);
    }
  }

  static function multi_column_sort($a, $b) {
    foreach ($a as $i => $sort) {
      if ($a[$i] == $b[$i]) {
        continue;
      }
      return $a[$i] < $b[$i] ? -1 : 1;
    }
    return 0;
  }
}
