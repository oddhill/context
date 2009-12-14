<?php
// $Id$

/**
 * Expose node views/node forms of specific node types as a context condition.
 */
class context_condition_node extends context_condition {
  function condition_values() {
    $values = array();
    foreach (node_get_types() as $type) {
      $values[$type->type] = $type->name;
    }
    return $values;
  }

  function options_form($context) {
    $defaults = $this->fetch_from_context($context, 'options');
    return array(
      'node_form' => array(
        '#title' => t('Set on node form'),
        '#type' => 'checkbox',
        '#description' => t('Set this context on node forms'),
        '#default_value' => isset($defaults['node_form']) ? $defaults['node_form'] : TRUE,
      ),
    );
  }

  function execute($node, $op) {
    $contexts = $this->get_contexts($node->type);
    $this->values[$node->type] = array();
    foreach ($contexts as $context) {
      // Check the node form option.
      if ($op === 'form') {
        $options = $this->fetch_from_context($context, 'options');
        if (!empty($options['node_form'])) {
          $this->values[$node->type][] = $context->name;
          context_set('context', $context->name, $context);
        }
      }
      else {
        $this->values[$node->type][] = $context->name;
        context_set('context', $context->name, $context);
      }
    }
  }
}
