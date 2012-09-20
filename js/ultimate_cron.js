jQuery(document).ready(function($) {
  $("a#ultimate-cron-show-all").click(function() {
    $(".ultimate-cron-admin-status").parent().show();
    return false;
  });
  $("a#ultimate-cron-show-error").click(function() {
    $(".ultimate-cron-admin-status:not(.ultimate-cron-admin-status-error)").parent().hide();
    $(".ultimate-cron-admin-status-error").parent().show();
    return false;
  });
  $("a#ultimate-cron-show-warning").click(function() {
    $(".ultimate-cron-admin-status:not(.ultimate-cron-admin-status-warning)").parent().hide();
    $(".ultimate-cron-admin-status-warning").parent().show();
    return false;
  });
  $("a#ultimate-cron-show-info").click(function() {
    $(".ultimate-cron-admin-status:not(.ultimate-cron-admin-status-info)").parent().hide();
    $(".ultimate-cron-admin-status-info").parent().show();
    return false;
  });
  $("a#ultimate-cron-show-success").click(function() {
    $(".ultimate-cron-admin-status:not(.ultimate-cron-admin-status-success)").parent().hide();
    $(".ultimate-cron-admin-status-success").parent().show();
    return false;
  });
  $("a#ultimate-cron-show-running").click(function() {
    $(".ultimate-cron-admin-status:not(.ultimate-cron-admin-status-running)").parent().hide();
    $(".ultimate-cron-admin-status-running").parent().show();
    return false;
  });
});
