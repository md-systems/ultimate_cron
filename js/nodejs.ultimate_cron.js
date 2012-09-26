(function ($) {


$(function() {
  // Ping Drupal for initial status of processes.
  $.ajax({
    type: 'GET',
    url: '/admin/ultimate-cron/service/process-status',
    data: '',
    dataType: 'json',
    success: function (result) {
      // Drupal.settings.ultimate_cron.initial_processes = result.processes;
      // Drupal.settings.ultimate_cron.processes = result.processes;
      // setTimeout(&ultimateCronInitialProcess, 100);
    }
  });

/**/
  // Setup progress counter
  setInterval(function() {
    var time = (new Date()).getTime() / 1000;
    $.each(Drupal.settings.ultimate_cron.processes, function (name, process) {
      // console.log(name + ' - ' + process.exec_status);
      if (process.exec_status == 2) {
        var row = 'row-' + name;
        var seconds = Math.round((time - Drupal.settings.ultimate_cron.skew) - process.start_time);
        seconds = seconds < 0 ? 0 : seconds;
        var formatted = (new Date(seconds * 1000)).toISOString().substring(11, 19);
        if (process.progress >= 0) {
          var progress = Math.round(process.progress * 100);
          formatted += ' (' + progress + '%)';
        }
        // console.log(formatted);
        $('tr.' + row + ' td.ultimate-cron-admin-status-running').closest('tr.' + row).find('td.ultimate-cron-admin-end').html(formatted);
      }
    });
  }, 1000);
/**/

});

Drupal.Nodejs.callbacks.nodejsBackgroundProcess = {
  updateSkew: function (time) {
    jsTime = (new Date()).getTime() / 1000;
    Drupal.settings.ultimate_cron.skew = jsTime - time;
  },

  callback: function (message) {
    var action = message.data.action;
    if (
      // action != 'locked' &&
      action != 'claimed' &&
      action != 'dispatch' &&
      // action != 'remove' &&
      action != 'setProgress' &&
      action != 'ultimateCronStatus'
    ) {
      return;
    }

    var process = message.data.background_process;
    var ultimate_cron = message.data.ultimate_cron;
    var name = process.handle.replace(/^uc:/, '');
    if (name == process.handle) {
      return;
    }
    console.log(name + ' ' + action + ' - ' + process.exec_status);
    Drupal.settings.ultimate_cron.processes[name] = process;

    var row = 'row-' + name;

    switch (action) {
      case 'dispatch':
        this.updateSkew(message.data.timestamp);
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('class', 'ultimate-cron-admin-status ultimate-cron-admin-status-running');
        $('tr.' + row + ' td.ultimate-cron-admin-start').html(ultimate_cron.start_time);
        $('tr.' + row + ' td.ultimate-cron-admin-end').html(Drupal.t('Starting'));
        $('tr.' + row + ' td.ultimate-cron-admin-status').html('<span>' + Drupal.t('Starting') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('title', Drupal.t('Running on !service_host', {'!service_host': process.service_host ? process.service_host : Drupal.t('N/A')}));
        var link = $('tr.' + row + ' td.ultimate-cron-admin-execute a');
        link.attr('href', ultimate_cron.unlockURL);
        link.html('<span>' + Drupal.t('Unlock') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-execute').attr('class', 'ultimate-cron-admin-unlock');
        break;
      case 'claimed':
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('class', 'ultimate-cron-admin-status ultimate-cron-admin-status-running');
        $('tr.' + row + ' td.ultimate-cron-admin-start').html(ultimate_cron.start_time);
        $('tr.' + row + ' td.ultimate-cron-admin-end').html('00:00:00');
        $('tr.' + row + ' td.ultimate-cron-admin-status').html('<span>' + Drupal.t('Running') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('title', Drupal.t('Running on !service_host', {'!service_host': process.service_host ? process.service_host : Drupal.t('N/A')}));
        var link = $('tr.' + row + ' td.ultimate-cron-admin-execute a');
        link.attr('href', ultimate_cron.unlockURL);
        link.html('<span>' + Drupal.t('Unlock') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-execute').attr('class', 'ultimate-cron-admin-unlock');
        break;
      case 'setProgress':
        break;
        this.updateSkew(message.data.timestamp);
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('class', 'ultimate-cron-admin-status ultimate-cron-admin-status-running');
        $('tr.' + row + ' td.ultimate-cron-admin-start').html(ultimate_cron.start_time);
        // $('tr.' + row + ' td.ultimate-cron-admin-end').html(ultimate_cron.progress);
        $('tr.' + row + ' td.ultimate-cron-admin-status').html('<span>' + Drupal.t('Running') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('title', Drupal.t('Running on !service_host', {'!service_host': process.service_host ? process.service_host : Drupal.t('N/A')}));
        var link = $('tr.' + row + ' td.ultimate-cron-admin-execute a');
        link.attr('href', ultimate_cron.unlockURL);
        link.html('<span>' + Drupal.t('Unlock') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-execute').attr('class', 'ultimate-cron-admin-unlock');
        break;
      case 'ultimateCronStatus':
        switch (parseInt(process.exec_status)) {
          case 1:
            message.data.action = 'dispatch';
            return this.callback(message);
          case 2:
            message.data.action = 'setProgress';
            return this.callback(message);
        }
        return;
      default:
        return;
    }

    var sel = location.hash.substring(1);
    sel = sel ? sel : 'show-all';
    $('a#ultimate-cron-' + sel).trigger('click');
  }
};

Drupal.Nodejs.callbacks.nodejsUltimateCron = {
  callback: function (message) {
    var action = message.data.action;
    switch (action) {
      case 'log':
        var log = message.data.log;
        // console.log(log.name + ' shutdown');
        delete Drupal.settings.ultimate_cron.processes[log.name];
        var row = 'row-' + log.name;
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('class', 'ultimate-cron-admin-status ultimate-cron-admin-status-' + log.formatted.severity);
        $('tr.' + row + ' td.ultimate-cron-admin-status').html('<span>' + log.severity + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-status').attr('title', log.formatted.msg ? log.formatted.msg : Drupal.t('No errors'));
        $('tr.' + row + ' td.ultimate-cron-admin-start').html(log.formatted.start_time);
        $('tr.' + row + ' td.ultimate-cron-admin-start').attr('title', Drupal.t('Previous run started @ !timestamp', {'!timestamp': log.formatted.start_time}));
        $('tr.' + row + ' td.ultimate-cron-admin-end').html(log.formatted.duration);
        $('tr.' + row + ' td.ultimate-cron-admin-end').attr('title', Drupal.t('Previous run finished @ !timestamp', {'!timestamp': log.formatted.end_time}));
        var link = $('tr.' + row + ' td.ultimate-cron-admin-unlock a');
        link.attr('href', log.formatted.executeURL);
        link.html('<span>' + Drupal.t('Run') + '</span>');
        $('tr.' + row + ' td.ultimate-cron-admin-unlock').attr('class', 'ultimate-cron-admin-execute');
        break;
      default:
        return;
    }

    var sel = location.hash.substring(1);
    sel = sel ? sel : 'show-all';
    $('a#ultimate-cron-' + sel).trigger('click');
  }
};

}(jQuery));

