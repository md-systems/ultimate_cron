<?php
/**
 * @file
 * Export-ui handler for the Ultimate Cron jobs.
 */

class ultimate_cron_job_ctools_export_ui extends ctools_export_ui {
  /**
   * Ensure we cannot add, import, delete or clone.
   */
  public function hook_menu(&$items) {
    parent::hook_menu($items);

    unset($items['admin/config/system/cron/jobs/add']);
    unset($items['admin/config/system/cron/jobs/import']);
    unset($items['admin/config/system/cron/jobs/list/%ctools_export_ui/delete']);
    unset($items['admin/config/system/cron/jobs/list/%ctools_export_ui/clone']);
  }

  /**
   * Ensure that we cannot clone from the operations link list.
   */
  public function build_operations($item) {
    $item->lock_id = isset($item->lock_id) ? $item->lock_id : $item->isLocked();
    $allowed_operations = parent::build_operations($item);
    unset($allowed_operations['clone']);
    if ($item->lock_id) {
      unset($allowed_operations['run']);
      $allowed_operations['unlock']['href'] .= '/' . $item->lock_id;
    }
    else {
      unset($allowed_operations['unlock']);
    }
    if (!empty($item->hook['configure'])) {
      $allowed_operations['configure'] = array(
        'title' => t('Configure'),
        'href' => $item->hook['configure'],
      );
    }
    $item->build_operations_alter($allowed_operations);
    return $allowed_operations;
  }

  /**
   * Custom action for plugins.
   */
  public function custom_page($js, $input, $item, $plugin_type, $plugin_name, $action) {
    $output = $item->custom_page($js, $input, $item, $plugin_type, $plugin_name, $action);
    if ($output) {
      return $output;
    }
    elseif (!$js) {
      drupal_goto(ctools_export_ui_plugin_base_path($this->plugin));
    }
    else {
      return $this->list_page($js, $input);
    }
  }

  /**
   * Run a job callback.
   */
  public function run_page($js, $input, $item) {
    $item->launch();
    if (!$js) {
      drupal_goto(ctools_export_ui_plugin_base_path($this->plugin));
    }
    else {
      return $this->list_page($js, $input);
    }
  }

  /**
   * Unlock a job callback.
   */
  public function unlock_page($js, $input, $item, $lock_id) {
    if ($item->unlock($lock_id, TRUE)) {
      $log = $item->loadLog($lock_id);
      $log->finished = FALSE;
      $log->catchMessages();
      global $user;
      $username = $user->uid ? $user->name : t('anonymous');
      watchdog('ultimate_cron', '@name manually unlocked by user @username (@uid)', array(
        '@name' => $item->name,
        '@username' => $username,
        '@uid' => $user->uid,
      ), WATCHDOG_WARNING);
      $log->finish();
    }

    if (!$js) {
      drupal_goto(ctools_export_ui_plugin_base_path($this->plugin));
    }
    else {
      return $this->list_page($js, $input);
    }
  }

  /**
   * Page with logs.
   */
  public function logs_page($js, $input, $item) {
    $output = '';
    $log_entries = $item->getLogEntries();
    $header = array(
      t('Started'),
      t('Duration'),
      t('User'),
      t('Initial message'),
      t('Message'),
      t('Status'),
    );

    $item->lock_id = isset($item->lock_id) ? $item->lock_id : $item->isLocked();
    $rows = array();
    foreach ($log_entries as $log_entry) {
      $rows[$log_entry->lid]['data'] = array();
      $start_time = $log_entry->start_time ? format_date((int) $log_entry->start_time, 'custom', 'Y-m-d H:i:s') : t('Never');
      $rows[$log_entry->lid]['data'][] = array('data' => $start_time, 'class' => array('ctools-export-ui-start-time'));

      $duration = NULL;
      if ($log_entry->start_time && $log_entry->end_time) {
        $duration = (int) ($log_entry->end_time - $log_entry->start_time);
      }
      elseif ($log_entry->start_time) {
        $duration = (int) (microtime(TRUE) - $log_entry->start_time);
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
      $rows[$log_entry->lid]['data'][] = array(
        'data' => $duration,
        'class' => array('ctools-export-ui-duration'),
        'title' => $log_entry->end_time ? t('Previous run finished @ @end_time', array(
          '@end_time' => format_date((int) $log_entry->end_time, 'custom', 'Y-m-d H:i:s'),
        )) : '',
      );

      $username = t('anonymous') . ' (0)';
      if ($log_entry->uid) {
        $user = user_load($log_entry->uid);
        $username = $user ? $user->name . " ($user->uid)": t('N/A');
      }
      $rows[$log_entry->lid]['data'][] = array('data' => $username, 'class' => array('ctools-export-ui-user'));

      $rows[$log_entry->lid]['data'][] = array('data' => '<pre>' . $log_entry->init_message . '</pre>', 'class' => array('ctools-export-ui-init-message'));
      $rows[$log_entry->lid]['data'][] = array('data' => '<pre>' . $log_entry->message . '</pre>', 'class' => array('ctools-export-ui-message'));

      // Status.
      if ($item->lock_id && $log_entry->lid == $item->lock_id) {
        $file = drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass.png';
        $status = theme('image', array('path' => $file));
        $title = t('running');
      }
      elseif ($log_entry->start_time && !$log_entry->end_time) {
        $file = drupal_get_path('module', 'ultimate_cron') . '/icons/lock_open.png';
        $status = theme('image', array('path' => $file));
        $title = t('unfinished but not locked?');
      }
      else {
        switch ($log_entry->severity) {
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

          default:
            $file = 'misc/message-16-ok.png';
        }
        $severity_levels = array(
          -1 => t('no info'),
        ) + watchdog_severity_levels();
        $status = theme('image', array('path' => $file));
        $title = $severity_levels[$log_entry->severity];
      }
      $rows[$log_entry->lid]['data'][] = array(
        'data' => $status,
        'class' => array('ctools-export-ui-status'),
        'title' => strip_tags($title),
      );

    }
    $output .= theme('table', array(
      'header' => $header,
      'rows' => $rows,
    ));
    $output .= theme('pager');
    return $output;
  }

  /**
   * Create the filter/sort form at the top of a list of exports.
   *
   * This handles the very default conditions, and most lists are expected
   * to override this and call through to parent::list_form() in order to
   * get the base form and then modify it as necessary to add search
   * gadgets for custom fields.
   */
  public function list_form(&$form, &$form_state) {
    parent::list_form($form, $form_state);

    $form['#attached']['js'][] = drupal_get_path('module', 'ultimate_cron') . '/js/ultimate_cron.js';

    // There's no normal for Ultimate Cron!
    unset($form['top row']['storage']['#options'][t('Normal')]);

    $all = array('all' => t('- All -'));

    $options = $all + array(
      'running' => 'running',
      -1 => 'no info',
    ) + watchdog_severity_levels();
    $form['top row']['status'] = array(
      '#type' => 'select',
      '#title' => t('Status'),
      '#options' => $options,
      '#default_value' => 'all',
      '#weight' => -2,
    );

    $jobs = ultimate_cron_get_hooks();
    $modules = array();
    foreach ($jobs as $job) {
      $info = system_get_info('module', $job['module']);
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
  public function list_filter($form_state, $item) {
    $schema = ctools_export_get_schema($this->plugin['schema']);
    if ($form_state['values']['storage'] != 'all' && $form_state['values']['storage'] != $item->{$schema['export']['export type string']}) {
      return TRUE;
    }

    if ($form_state['values']['module'] != 'all' && $form_state['values']['module'] != $item->hook['module']) {
      return TRUE;
    }

    $item->log_entry = $item->loadLatestLog()->log_entry;
    $item->lock_id = isset($item->lock_id) ? $item->lock_id : $item->isLocked();

    if ($form_state['values']['status'] == 'running') {
      if (!$item->lock_id) {
        return TRUE;
      }
    }
    elseif ($form_state['values']['status'] != 'all' && $form_state['values']['status'] != $item->log_entry->severity) {
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
  public function list_table_header() {
    $header = array();
    $header[] = array('data' => t('Module'), 'class' => array('ctools-export-ui-module'));
    if (!empty($this->plugin['export']['admin_title'])) {
      $header[] = array('data' => t('Title'), 'class' => array('ctools-export-ui-title'));
    }

    $header[] = array('data' => t('Scheduled'), 'class' => array('ctools-export-ui-scheduled'));
    $header[] = array('data' => t('Started'), 'class' => array('ctools-export-ui-start-time'));
    $header[] = array('data' => t('Duration'), 'class' => array('ctools-export-ui-duration'));
    $header[] = array('data' => t('Status'), 'class' => array('ctools-export-ui-status'));
    $header[] = array('data' => t('Storage'), 'class' => array('ctools-export-ui-storage'));
    $header[] = array('data' => t('Operations'), 'class' => array('ctools-export-ui-operations'));

    return $header;
  }

  /**
   * Provide a list of sort options.
   *
   * Override this if you wish to provide more or change how these work.
   * The actual handling of the sorting will happen in build_row().
   */
  public function list_sort_options() {
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
  public function list_build_row($item, &$form_state, $operations) {
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
    $log_entry = $item->log_entry;
    $start_time = $log_entry->start_time ? format_date((int) $log_entry->start_time, 'custom', 'Y-m-d H:i:s') : t('Never');

    $username = t('anonymous') . ' (0)';
    if ($log_entry->uid) {
      $user = user_load($log_entry->uid);
      $username = $user ? $user->name . " ($user->uid)": t('N/A');
    }

    $this->rows[$name]['data'][] = array(
      'data' => $start_time,
      'class' => array('ctools-export-ui-last-start-time'),
      'title' => strip_tags($log_entry->init_message) . ' ' . t('by') . " $username",
    );

    $duration = NULL;
    if ($log_entry->start_time && $log_entry->end_time) {
      $duration = (int) ($log_entry->end_time - $log_entry->start_time);
    }
    elseif ($log_entry->start_time) {
      $duration = (int) (microtime(TRUE) - $log_entry->start_time);
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
      'title' => $log_entry->end_time ? t('Previous run finished @ @end_time', array(
        '@end_time' => format_date((int) $log_entry->end_time, 'custom', 'Y-m-d H:i:s'),
      )) : '',
    );

    // Status.
    if ($item->lock_id && $log_entry->lid == $item->lock_id) {
      $file = drupal_get_path('module', 'ultimate_cron') . '/icons/hourglass.png';
      $status = theme('image', array('path' => $file));
      $title = t('running');
    }
    elseif ($log_entry->start_time && !$log_entry->end_time) {
      $file = drupal_get_path('module', 'ultimate_cron') . '/icons/lock_open.png';
      $status = theme('image', array('path' => $file));
      $title = t('unfinished but not locked?');
    }
    else {
      switch ($log_entry->severity) {
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

        default:
          $file = 'misc/message-16-ok.png';
      }
      $severity_levels = array(
        -1 => t('no info'),
      ) + watchdog_severity_levels();
      $status = theme('image', array('path' => $file));
      $title = $log_entry->message ? $log_entry->message : $severity_levels[$log_entry->severity];
    }
    $this->rows[$name]['data'][] = array(
      'data' => $status,
      'class' => array('ctools-export-ui-status'),
      'title' => strip_tags($title),
    );


    // Storage.
    $this->rows[$name]['data'][] = array('data' => check_plain($item->{$schema['export']['export type string']}), 'class' => array('ctools-export-ui-storage'));

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
  public function list_form_submit(&$form, &$form_state) {
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

  /**
   * Sort callback for multiple column sort.
   */
  static public function multi_column_sort($a, $b) {
    foreach ($a as $i => $sort) {
      if ($a[$i] == $b[$i]) {
        continue;
      }
      return $a[$i] < $b[$i] ? -1 : 1;
    }
    return 0;
  }
}
