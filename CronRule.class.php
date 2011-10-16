<?php
/**
 * @file
 *
 * by Thomas Gielfeldt
 * <thomas@gielfeldt.com>
 *
 * This class parses cron rules and determines last execution time using least case integer comparison.
 *
 */

class CronRule {

  public $rule = NULL;
  public $allow_shorthand = FALSE;

  /**
   * Constructor
   */
  function __construct($rule) {
    $this->rule = $rule;
  }

  /**
   * Expand interval from cronrule part
   *
   * @param $matches (e.g. 4-43/5+2)
   *   array of matches:
   *     [1] = lower
   *     [2] = upper
   *     [5] = step
   *     [7] = offset
   *
   * @return
   *   (string) comma-separated list of values
   */
  function expandInterval($matches) {
    $result = array();
    $matches[5] = isset($matches[5]) ? $matches[5] : 1;
    $matches[7] = isset($matches[7]) ? $matches[7] : 0;
    if ($matches[5] <= 0) return '';
    $step = ($matches[5] > 0) ? $matches[5] : 1;
    for ($i = $matches[1]; $i <= $matches[2]; $i+=$step) {
      $result[] = ($i + $matches[7]) % ($matches[2] + 1);
    }
    return implode(',', $result);
  }

  /**
   * Expand range from cronrule part
   *
   * @param $rule
   *   (string) cronrule part, e.g.: 1,2,3,4-43/5
   * @param $max
   *   (string) boundaries, e.g.: 0-59
   * @param $digits
   *   (int) number of digits of value (leading zeroes)
   * @return
   *   (array) array of valid values
   */
  function expandRange($rule, $max) {
    $rule = str_replace("*", $max, $rule);
    $rule = preg_replace_callback('/(\d+)-(\d+)((\/(\d+))(\+(\d+))?)?/', array($this, 'expandInterval'), $rule);
    if (!preg_match('/([^0-9\,])/', $rule)) {
      $rule = explode(',', $rule);
      rsort($rule);
    }
    else {
      $rule = array();
    }
    return $rule;
  }
  /**
   * Generate regex rules
   *
   * @param $rule
   *   (string) cronrule, e.g: 1,2,3,4-43/5 * * * 2,5
   * @return
   *   (array) date and time regular expression for mathing rule
   */
  function getIntervals($rule) {
    $parts = preg_split('/\s+/', $rule);
    if ($this->allow_shorthand) $parts += array('*', '*', '*', '*', '*'); // Allow short rules by appending wildcards?
    if (count($parts) != 5) return FALSE;
    $intervals = array();
    $intervals['minutes']  = $this->expandRange($parts[0], '0-59');
    if (empty($intervals['minutes'])) return FALSE;
    $intervals['hours']    = $this->expandRange($parts[1], '0-23');
    if (empty($intervals['hours'])) return FALSE;
    $intervals['days']     = $this->expandRange($parts[2], '1-31');
    if (empty($intervals['days'])) return FALSE;
    $intervals['months']   = $this->expandRange($parts[3], '1-12');
    if (empty($intervals['months'])) return FALSE;
    $intervals['weekdays'] = $this->expandRange($parts[4], '0-6');
    if (empty($intervals['weekdays'])) return FALSE;
    $intervals['weekdays'] = array_flip($intervals['weekdays']);

    return $intervals;
  }

  /**
   * Get last execution time of rule in unix timestamp format
   *
   * @param $time
   *   (int) time to use as relative time (default now)
   * @return
   *   (int) unix timestamp of last execution time
   */
  function getLastRan($time = NULL) {
    // Current time round to last minute
    if (!isset($time)) $time = time();
    $time = floor($time / 60) * 60;

    // Generate regular expressions from rule
    $intervals = $this->getIntervals($this->rule);
    if ($intervals === FALSE) return FALSE;

    // Get starting points
    $start_year   = date('Y', $time);
    $end_year     = $start_year - 28; // Go back max 28 years (leapyear * weekdays)
    $start_month  = date('n', $time);
    $start_day    = date('j', $time);
    $start_hour   = date('G', $time);
    $start_minute = (int)date('i', $time);

    // If both weekday and days are restricted, then use either or
    // otherwise, use and ... when using or, we have to try out all the days in the month
    // and not just to the ones restricted
    $check_both = (count($intervals['days']) != 31 && count($intervals['weekdays']) != 7) ? FALSE : TRUE;
    $days = $check_both ? $intervals['days'] : range(31, 1);

    // Find last date and time this rule was run
    for ($year = $start_year; $year > $end_year; $year--) {
      foreach ($intervals['months'] as $month) {
        if ($month < 1 || $month > 12) return FALSE;
        if ($year >= $start_year && $month > $start_month) continue;

        foreach ($days as $day) {
          if ($day < 1 || $day > 31) return FALSE;
          if ($year >= $start_year && $month >= $start_month && $day > $start_day) continue;
          if (!checkdate($month, $day, $year)) continue;

          // Check days and weekdays using and/or logic
          if ($check_both) {
            if (!isset($intervals['weekdays'][jddayofweek(gregoriantojd($month, $day, $year), 0)])) continue;
          }
          else {
            if (
              !in_array($day, $intervals['days']) &&
              !isset($intervals['weekdays'][jddayofweek(gregoriantojd($month, $day, $year), 0)])
            ) continue;
          }

          if ($day != $start_day || $month != $start_month || $year != $start_year) {
            $start_hour = 23;
            $start_minute = 59;
          }
          foreach ($intervals['hours'] as $hour) {
            if ($hour < 0 || $hour > 23) return FALSE;
            if ($hour > $start_hour) continue;
            if ($hour < $start_hour) $start_minute = 59;
            foreach ($intervals['minutes'] as $minute) {
              if ($minute < 0 || $minute > 59) return FALSE;
              if ($minute > $start_minute) continue;
              break 5;
            }
          }
        }
      }
    }

    // Create unix timestamp from derived date+time
    $time = mktime($hour, $minute, 0, $month, $day, $year);

    return $time;
  }

  /**
   * Check if a rule is valid
   */
  function isValid($time = NULL) {
    return $this->getLastRan($time) === FALSE ? FALSE : TRUE;
  }
}
