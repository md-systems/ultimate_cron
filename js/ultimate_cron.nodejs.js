(function ($) {

Drupal.Nodejs.callbacks.nodejsUltimateCron = {
  callback: function (message) {
    var action = message.data.action;
    var job = message.data.job;
    console.log(action);
    console.log(job);
  }
};

}(jQuery));

