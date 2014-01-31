<?php
/**
 * @file
 * This class parses cron rules and determines last execution time using least case integer comparison.
 */

class CronRule {

  public $rule = NULL;
  public $time = NULL;
  public $offset = 0;

  public $allow_shorthand = FALSE;
  private static $ranges = array(
    'minutes' => array(0, 59),
    'hours' => array(0, 23),
    'days' => array(1, 31),
    'months' => array(1, 12),
    'weekdays' => array(0, 6),
  );

  private $parsed_rule = array();
  private $type = NULL;
  static private $instances = array();

  /**
   * Factory method for CronRule instance.
   *
   * @param string $rule
   *   The crontab rule to use.
   * @param integer $time
   *   The time to test against.
   * @param integer $offset
   *   Offset for @ flag.
   *
   * @return CronRule
   *   CronRule object.
   */
  public function factory($rule, $time = NULL, $offset = 0) {
    if (strpos($rule, '@') === FALSE) {
      $offset = 0;
    }

    $time = isset($time) ? $time : time();

    $key = "$rule:$time:$offset";
    if (isset(self::$instances[$key])) {
      return self::$instances[$key];
    }
    self::$instances[$key] = new CronRule($rule, $time, $offset);
    return self::$instances[$key];
  }

  /**
   * Constructor.
   *
   * @param string $rule
   *   The crontab rule to use.
   * @param integer $time
   *   The time to test against.
   * @param integer $offset
   *   Offset for @ flag.
   */
  public function __construct($rule, $time, $offset) {
    $this->rule = $rule;
    $this->time = $time;
    $this->offset = $offset;
  }

  /**
   * Expand interval from cronrule part.
   *
   * @param array $matches
   *   (e.g. 4-43/5+2).
   *   array of matches:
   *     [1] = lower
   *     [2] = upper
   *     [5] = step
   *     [7] = offset
   *
   * @return string
   *   Comma-separated list of values.
   */
  public function expandInterval($matches) {
    $result = array();

    $lower = $matches[1];
    $upper = isset($matches[2]) && $matches[2] != '' ? $matches[2] : $lower;
    $step = isset($matches[5]) && $matches[5] != '' ? $matches[5] : 1;
    $offset = isset($matches[7]) && $matches[7] != '' ? $matches[7] : 0;

    if ($step <= 0) {
      return '';
    }

    $step = ($step > 0) ? $step : 1;
    for ($i = $lower; $i <= $upper; $i += $step) {
      $result[] = ($i + $offset) % (self::$ranges[$this->type][1] + 1);
    }
    return implode(',', $result);
  }

  /**
   * Expand range from cronrule part.
   *
   * @param string $rule
   *   Cronrule part, e.g.: 1,2,3,4-43/5.
   * @param string $type
   *   Type of range (minutes, hours, etc.)
   *
   * @return array
   *   Valid values for this range.
   */
  public function expandRange($rule, $type) {
    $this->type = $type;
    $max = implode('-', self::$ranges[$type]);
    $rule = str_replace("*", $max, $rule);
    $rule = str_replace("@", $this->offset % (self::$ranges[$type][1] + 1), $rule);
    $this->parsed_rule[$type] = $rule;
    $rule = preg_replace_callback('!(\d+)(?:-(\d+))?((/(\d+))?(\+(\d+))?)?!', array($this, 'expandInterval'), $rule);
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
   *   Parts of rules to pre process.
   */
  public function preProcessRule(&$parts) {
    // Allow JAN-DEC.
    $months = array(1 => 'jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov', 'dec');
    $parts[3] = strtr(strtolower($parts[3]), array_flip($months));

    // Allow SUN-SUN.
    $days = array('sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat');
    $parts[4] = strtr(strtolower($parts[4]), array_flip($days));
    $parts[4] = str_replace('7', '0', $parts[4]);
  }

  /**
   * Post process rule.
   *
   * @param array &$intervals
   *   Intervals to post process.
   */
  public function postProcessRule(&$intervals) {
  }

  /**
   * Generate regex rules.
   *
   * @param string $rule
   *   Cronrule, e.g: 1,2,3,4-43/5 * * * 2,5.
   *
   * @return array
   *   Date and time regular expression for mathing rule.
   */
  public function getIntervals($rule = NULL) {
    $parts = preg_split('/\s+/', isset($rule) ? $rule : $this->rule);
    if ($this->allow_shorthand) {
      // Allow short rules by appending wildcards?
      $parts += array('*', '*', '*', '*', '*');
    }
    if (count($parts) != 5) {
      return FALSE;
    }
    $this->preProcessRule($parts);
    $intervals = array();
    $intervals['minutes']  = $this->expandRange($parts[0], 'minutes');
    if (empty($intervals['minutes'])) {
      return FALSE;
    }
    $intervals['hours']    = $this->expandRange($parts[1], 'hours');
    if (empty($intervals['hours'])) {
      return FALSE;
    }
    $intervals['days']     = $this->expandRange($parts[2], 'days');
    if (empty($intervals['days'])) {
      return FALSE;
    }
    $intervals['months']   = $this->expandRange($parts[3], 'months');
    if (empty($intervals['months'])) {
      return FALSE;
    }
    $intervals['weekdays'] = $this->expandRange($parts[4], 'weekdays');
    if (empty($intervals['weekdays'])) {
      return FALSE;
    }
    $intervals['weekdays'] = array_flip($intervals['weekdays']);
    $this->postProcessRule($intervals);

    return $intervals;
  }

  /**
   * Convert intervals back into crontab rule format.
   *
   * @param array $intervals
   *   Intervals to convert.
   *
   * @return string
   *   Crontab rule.
   */
  public function rebuildRule($intervals) {
    $parts = array();
    foreach ($intervals as $type => $interval) {
      $parts[] = $this->parsed_rule[$type];
    }
    return implode(' ', $parts);
  }

  /**
   * Parse rule. Run through parser expanding expression, and recombine into crontab syntax.
   */
  public function parseRule() {
    if (isset($this->parsed)) {
      return $this->parsed;
    }
    $this->parsed = $this->rebuildRule($this->getIntervals());
    return $this->parsed;
  }

  /**
   * Get last execution time of rule in UNIX timestamp format.
   *
   * @return integer
   *   UNIX timestamp of last execution time
   */
  public function getLastRan() {
    if (isset($this->last_ran)) {
      return $this->last_ran;
    }
    error_log("PARSING: $this->rule : $this->time : $this->offset");

    // Current time round to last minute.
    $time = floor($this->time / 60) * 60;

    // Generate regular expressions from rule.
    $intervals = $this->getIntervals();
    if ($intervals === FALSE) {
      return FALSE;
    }

    // Get starting points.
    $start_year   = date('Y', $time);
    // Go back max 28 years (leapyear * weekdays).
    $end_year     = $start_year - 28;
    $start_month  = date('n', $time);
    $start_day    = date('j', $time);
    $start_hour   = date('G', $time);
    $start_minute = (int) date('i', $time);

    // If both weekday and days are restricted, then use either or
    // otherwise, use and ... when using or, we have to try out all the days in the month
    // and not just to the ones restricted.
    $check_both = (count($intervals['days']) != 31 && count($intervals['weekdays']) != 7) ? FALSE : TRUE;
    $days = $check_both ? $intervals['days'] : range(31, 1);

    // Find last date and time this rule was run.
    for ($year = $start_year; $year > $end_year; $year--) {
      foreach ($intervals['months'] as $month) {
        if ($month < 1 || $month > 12) {
          continue;
        }
        if ($year >= $start_year && $month > $start_month) {
          continue;
        }

        foreach ($days as $day) {
          if ($day < 1 || $day > 31) {
            continue;
          }
          if ($year >= $start_year && $month >= $start_month && $day > $start_day) {
            continue;
          }
          if (!checkdate($month, $day, $year)) {
            continue;
          }

          // Check days and weekdays using and/or logic.
          $date_array = getdate(mktime(0, 0, 0, $month, $day, $year));
          if ($check_both) {
            if (!isset($intervals['weekdays'][$date_array['wday']])) {
              continue;
            }
          }
          else {
            if (
              !in_array($day, $intervals['days']) &&
              !isset($intervals['weekdays'][$date_array['wday']])
            ) {
              continue;
            }
          }

          if ($day != $start_day || $month != $start_month || $year != $start_year) {
            $start_hour = 23;
            $start_minute = 59;
          }
          foreach ($intervals['hours'] as $hour) {
            if ($hour < 0 || $hour > 23) {
              continue;
            }
            if ($hour > $start_hour) {
              continue;
            }
            if ($hour < $start_hour) {
              $start_minute = 59;
            }
            foreach ($intervals['minutes'] as $minute) {
              if ($minute < 0 || $minute > 59) {
                continue;
              }
              if ($minute > $start_minute) {
                continue;
              }
              break 5;
            }
          }
        }
      }
    }

    // Create UNIX timestamp from derived date+time.
    $this->last_ran = mktime($hour, $minute, 0, $month, $day, $year);

    return $this->last_ran;
  }

  /**
   * Check if a rule is valid.
   */
  public function isValid() {
    return $this->getLastRan() === FALSE ? FALSE : TRUE;
  }
}
