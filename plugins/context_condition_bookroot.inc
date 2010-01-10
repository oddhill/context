<?php
// $Id$

/**
 * Set the context on the basis of the node type of page's book root.
 */
class context_condition_bookroot extends context_condition_node {
  function execute($node, $op) {
    if (!empty($node->book['bid'])) {
      $type = db_result(db_query("SELECT type FROM {node} WHERE nid = %d", $node->book['bid']));
      $book = new stdClass();
      $book->type = $type;
      parent::execute($book, $op);
    }
  }
}
