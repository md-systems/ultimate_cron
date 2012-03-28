<?php
/**
 * @file
 *
 * This class parses cron rules and determines last execution time using least case integer comparison.
 */

class CronRule {

  public $rule = NULL;
  public $allow_shorthand = FALSE;
  private static $ranges = array(
    'minutes' => array(0, 59),
    'hours' => array(0, 23),
    'days' => array(1, 31),
    'months' => array(1, 12),
    'weekdays' => array(0, 6),
  );

  private $parsed_rule = array();
  public $offset = 0;

  /**
   * Constructor
   */
  function __construct($rule = NULL) {
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

    $lower = $matches[1];
    $upper = $matches[2];
    $step = isset($matches[5]) ? $matches[5] : 1;
    $offset = isset($matches[7]) ? $matches[7] : 0;

    if ($step <= 0) return '';
    $step = ($step > 0) ? $step : 1;
    for ($i = $lower; $i <= $upper; $i+=$step) {
      $result[] = ($i + $offset) % ($upper + 1);
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
  function expandRange($rule, $type) {
    $max = implode('-', self::$ranges[$type]);
    $rule = str_replace("*", $max, $rule);
    $rule = str_replace("@", $this->offset % (self::$ranges[$type][1] + 1), $rule);
    $this->parsed_rule[$type] = $rule;
    $rule = preg_replace_callback('!(\d+)-(\d+)((/(\d+))?(\+(\d+))?)?!', array($this, 'expandInterval'), $rule);
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
   * Pre process rule.
   *
   * @param array &$parts
   */
  function preProcessRule(&$parts) {
    // Allow JAN-DEC
    $months = array(1 => 'jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov', 'dec');
    $parts[3] = strtr(strtolower($parts[3]), array_flip($months));

    // Allow SUN-SUN
    $days = array('sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat');
    $parts[4] = strtr(strtolower($parts[4]), array_flip($days));
    $parts[4] = str_replace('7', '0', $parts[4]);
  }

  /**
   * Post process rule
   *
   * @param array $intervals
   */
  function postProcessRule(&$intervals) {
  }

  /**
   * Generate regex rules
   *
   * @param $rule
   *   (string) cronrule, e.g: 1,2,3,4-43/5 * * * 2,5
   * @return
   *   (array) date and time regular expression for mathing rule
   */
  function getIntervals($rule = NULL) {
    $parts = preg_split('/\s+/', isset($rule) ? $rule : $this->rule);
    if ($this->allow_shorthand) $parts += array('*', '*', '*', '*', '*'); // Allow short rules by appending wildcards?
    if (count($parts) != 5) return FALSE;
    $this->preProcessRule($parts);
    $intervals = array();
    $intervals['minutes']  = $this->expandRange($parts[0], 'minutes');
    if (empty($intervals['minutes'])) return FALSE;
    $intervals['hours']    = $this->expandRange($parts[1], 'hours');
    if (empty($intervals['hours'])) return FALSE;
    $intervals['days']     = $this->expandRange($parts[2], 'days');
    if (empty($intervals['days'])) return FALSE;
    $intervals['months']   = $this->expandRange($parts[3], 'months');
    if (empty($intervals['months'])) return FALSE;
    $intervals['weekdays'] = $this->expandRange($parts[4], 'weekdays');
    if (empty($intervals['weekdays'])) return FALSE;
    $intervals['weekdays'] = array_flip($intervals['weekdays']);
    $this->postProcessRule($intervals);

    return $intervals;
  }

  /**
   * Convert intervals back into crontab rule format
   */
  function rebuildRule($intervals) {
    $parts = array();
    foreach ($intervals as $type => $interval) {
      $parts[] = $this->parsed_rule[$type];
    }
    return implode(' ', $parts);
  }

  /**
   * Parse rule. Run through parser expanding expression, and recombine into crontab syntax.
   */
  function parseRule() {
    return $this->rebuildRule($this->getIntervals());
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
    $intervals = $this->getIntervals();
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
        if ($month < 1 || $month > 12) continue;
        if ($year >= $start_year && $month > $start_month) continue;

        foreach ($days as $day) {
          if ($day < 1 || $day > 31) continue;
          if ($year >= $start_year && $month >= $start_month && $day > $start_day) continue;
          if (!checkdate($month, $day, $year)) continue;

          // Check days and weekdays using and/or logic
          $date_array = getdate(mktime(0, 0, 0, $month, $day, $year));
          if ($check_both) {
            if (!isset($intervals['weekdays'][$date_array['wday']])) continue;
          }
          else {
            if (
              !in_array($day, $intervals['days']) &&
              !isset($intervals['weekdays'][$date_array['wday']])
            ) continue;
          }

          if ($day != $start_day || $month != $start_month || $year != $start_year) {
            $start_hour = 23;
            $start_minute = 59;
          }
          foreach ($intervals['hours'] as $hour) {
            if ($hour < 0 || $hour > 23) continue;
            if ($hour > $start_hour) continue;
            if ($hour < $start_hour) $start_minute = 59;
            foreach ($intervals['minutes'] as $minute) {
              if ($minute < 0 || $minute > 59) continue;
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
