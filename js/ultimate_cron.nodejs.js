(function ($) {
  Drupal.Nodejs.callbacks.nodejsUltimateCron = {
    disabled: false,
    runningJobs: {},
    callback: function (message) {
      if (this.disabled) return;
      var action = message.data.action;
      var job = message.data.job;
      var elements = message.data.elements;

      console.log(action + ' ' + job.name);

      switch (action) {
        case 'lock':
          job.started = new Date().getTime();
          this.runningJobs[job.name] = job;
          break;

        case 'unlock':
          delete(this.runningJobs[job.name]);
          // console.log(elements);
          break;

      }
      // console.log(elements);
      for (var key in elements) {
        if (elements.hasOwnProperty(key)) {
          var value = elements[key];
          $(key).html(value);
          // console.log(value);
          console.log('attachBehavior: ' + key);
          Drupal.attachBehaviors($(key));
        }
      }

/*
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
*/
    }
  };

  Drupal.behaviors.ultimateCronJobNodejs = {
    attach: function (context) {
      // console.log(context);
      Drupal.Nodejs.callbacks.nodejsUltimateCron.runningJobs = {};
      $("td.ctools-export-ui-status", context).each(function() {
        var row = $(this).parent('tr');
        var name = $(row).attr('id');
        if ($(this).attr('title') == 'running') {
          // console.log('Starting: ' + name);
          var duration = $("tr#" + name + " td.ctools-export-ui-duration span.duration-time").attr('data-src');
          Drupal.Nodejs.callbacks.nodejsUltimateCron.runningJobs[name] = {
            started: (new Date().getTime()) - (duration * 1000),
          };
        }
        else {
          // console.log('Stopping: ' + name);
          delete(Drupal.Nodejs.callbacks.nodejsUltimateCron.runningJobs[name]);
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

