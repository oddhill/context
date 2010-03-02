<?php
// $Id$

/**
 * Expose current user role as a context condition.
 */
class context_condition_user extends context_condition {
  function condition_values() {
    return drupal_map_assoc(user_roles());
  }

  function execute($account) {
    $roles = $account->roles;
    foreach ($roles as $role) {
      foreach ($this->get_contexts($role) as $context) {
        $this->condition_met($context, $role);
      }
    }
  }
}
