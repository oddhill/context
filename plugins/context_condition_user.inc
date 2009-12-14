<?php
// $Id$

/**
 * Expose node user pages as context conditions.
 */
class context_condition_user extends context_condition {
  function condition_values() {
    return drupal_map_assoc(user_roles(TRUE));
  }

  function execute($account, $op) {
    if ($op == 'view') {
      $roles = $account->roles;
      foreach ($roles as $role) {
        $this->values[$role] = array();
        if ($contexts = $this->get_contexts($role)) {
          foreach ($contexts as $context) {
            $this->values[$role][] = $context->name;
            context_set('context', $context->name, $context);
          }
        }
      }
    }
  }
}
