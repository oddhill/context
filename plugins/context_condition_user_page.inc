<?php

/**
 * Expose user pages as a context condition.
 */
class context_condition_user_page extends context_condition {
  function condition_values() {
    $values = array();
    $values['view'] = t('User profile');
    $values['form'] = t('User account form');
    $values['register'] = t('Registration form');
    return $values;
  }

  function execute($op) {
    foreach ($this->get_contexts($op) as $context) {
      $this->condition_met($context);
    }
  }
}
