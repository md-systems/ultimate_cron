(function ($) {
  Drupal.Nodejs.callbacks.nodejsUltimateCron = {
    runningJobs: {},
    callback: function (message) {
      var action = message.data.action;
      var job = message.data.job;
      var elements = message.data.elements;
      switch (action) {
        case 'lock':
          job.started = new Date().getTime();
          this.runningJobs[job.name] = job;
          break;

        case 'unlock':
          delete(this.runningJobs[job.name]);
          break;

      }

      for (var key in elements) {
        if (elements.hasOwnProperty(key)) {
          var value = elements[key];
          if (typeof value == 'string') {
            $(key).html(value);
          }
          else {
            for (var attr in value) {
              if (attr == 'data') {
                $(key).html(value.data);
              }
              else if (value.hasOwnProperty(attr)) {
                $(key).attr(attr, value[attr]);
              }
            }
          }

        }
      }
    }
  };

  Drupal.behaviors.ultimateCronJobNodejs = {
    attach: function (context) {
      Drupal.Nodejs.callbacks.nodejsUltimateCron.runningJobs = {};
      $("tr td.ctools-export-ui-status").each(function() {
        if ($(this).attr('title') == 'running') {
          var row = $(this).parent('tr');
          var name = $(row).attr('id');
          var duration = $("tr#" + name + " td.ctools-export-ui-duration span.duration-time").attr('data-src');
          Drupal.Nodejs.callbacks.nodejsUltimateCron.runningJobs[name] = {
            started: (new Date().getTime()) - (duration * 1000),
          };
        }
      });
    }
  };

  setInterval(function() {
    var time = new Date().getTime();
    var jobs = Drupal.Nodejs.callbacks.nodejsUltimateCron.runningJobs;
    for (var name in jobs) {
      if (jobs.hasOwnProperty(name)) {
        var job = jobs[name];
        var date = new Date(time - job.started);
        var minutes = '00' + date.getUTCMinutes();
        var seconds = '00' + date.getUTCSeconds();
        var formatted = minutes.substring(minutes.length - 2) + ':' + seconds.substring(seconds.length - 2);
        $("tr#" + name + " td.ctools-export-ui-duration .duration-time").html(formatted);
      }
    }
  }, 1000)

}(jQuery));

