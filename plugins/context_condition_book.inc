<?php
// $Id$

/**
 * Expose book properties as a context condition.
 */
class context_condition_book extends context_condition {
  function condition_values() {
    $values = array();
    foreach (book_get_books() as $book) {
      $values[$book['menu_name']] = $book['title'];
    }
    return $values;
  }

  function execute($node, $op) {
    if (isset($node->book, $node->book['menu_name'])) {
      $this->values[$node->book['menu_name']] = array();
      $contexts = $this->get_contexts($node->book['menu_name']);
      foreach ($contexts as $context) {
        $this->values[$node->book['menu_name']][] = $context->name;
        context_set('context', $context->name, $context);
      }
    }
  }
}
