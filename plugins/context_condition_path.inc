<?php
// $Id$

/**
 * Expose paths as a context condition.
 */
class context_condition_path extends context_condition {
  /**
   * Omit condition values. We will provide a custom input form for our conditions.
   */
  function condition_values() {
    return array();
  }

  /**
   * Condition form.
   */
  function condition_form($context) {
    $form = parent::condition_form($context);
    unset($form['#options']);

    $form['#type'] = 'textarea';
    $form['#default_value'] = implode("\n", $this->fetch_from_context($context, 'values'));
    return $form;
  }

  /**
   * Condition form submit handler.
   */
  function condition_form_submit($values) {
    $parsed = array();
    $items = explode("\n", $values);
    if (!empty($items)) {
      foreach ($items as $v) {
        $v = trim($v);
        if (!empty($v)) {
          $parsed[$v] = $v;
        }
      }
    }
    return $parsed;
  }

  /**
   * Execute.
   */
  function execute() {
    if ($this->condition_used()) {
      $current_path = drupal_get_path_alias($_GET['q']);
      foreach ($this->get_contexts() as $context) {
        foreach ($this->fetch_from_context($context, 'values') as $path) {
          $path_match = FALSE;
          if ($path_match = drupal_match_path($current_path, $path)) {
            $this->condition_met($context, $path);
          }
          else if ($current_path != $_GET['q'] && drupal_match_path($_GET['q'], $path)) {
            $this->condition_met($context, $path);
          }
        }
      }
    }
  }
}
