<?php
// $Id$

/**
 * Expose current language as a context condition.
 */
class context_condition_language extends context_condition {
  function condition_values() {
    return module_exists('locale') ? locale_language_list() : array();
  }
}
