
Credits
-------

Thanks to Mark James for the icons
  http://www.famfamfam.com/lab/icons/silk/


Example code:


// Default cron-function, configurable through /admin/build/cron/settings
function mymodule_cron() {
  // Do some stuff ...
}


// Define custom cron functions
function mymodule_cronapi($op, $job = NULL) {
  switch($op) {
    case 'list':
      return array(
        'mymodule_cronjob_1' => 'Cron-1 Handler',
        'mymodule_cronjob_2' => 'Cron-2 Handler',
        'mymodule_cronjob_3' => 'Cron-3 Handler',
      );

    case 'rule':
      switch($job) {
        case 'mymodule_cronjob_1': return '*/13 * * * *';
        case 'mymodule_cronjob_2': return '0 0 1 * *';
      );

    case 'execute':
      switch($job) {
        case 'mymodule_cronjob_2':
          mymodule_somefunction();
          break;
      }

  }
}

// Custom cron-function
function mymodule_cronjob_1() {
  // Do some stuff ...
}

// Custom cron-function
function mymodule_somefunction() {
  // Do some stuff ...
}

// Custom cron-function
function mymodule_cronjob_3() {
  // Do some stuff ...
}

// Easy-hook, uses rule: 0 * * * *
function mymodule_hourly() {
  // Do some stuff
}

// Easy-hook, uses rule: 0 0 * * *
function mymodule_daily() {
  // Do some stuff
}

// Easy-hook, uses rule: 0 0 * * 1
function mymodule_weekly() {
  // Do some stuff
}

// Easy-hook, uses rule: 0 0 1 * *
function mymodule_monthly() {
  // Do some stuff
}

// Easy-hook, uses rule: 0 0 1 1 *
function mymodule_yearly() {
  // Do some stuff
}


