<?php
// $Id$

/**
 * Simple sitewide context, always present.
 */
class context_condition_sitewide extends context_condition {
  function condition_values() {
    return array(1 => t('Always active'));
  }

  function editor_form($context = NULL) {
    $form = parent::editor_form($context);
    $form[1]['#title'] = t('Always active');
    $form['#weight'] = -10;
    return $form;
  }
}
