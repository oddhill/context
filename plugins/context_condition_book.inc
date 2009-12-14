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
    $this->values[] = $node;
    if (isset($node->book, $node->book['menu_name'])) {
      $contexts = $this->get_contexts($node->book['menu_name']);
      foreach ($contexts as $context) {
        context_set('context', $context->name, $context);
      }
    }
  }
}
