<?php
// $Id$

/**
 * Set the breadcrumb using a context reaction.
 */
class context_reaction_breadcrumb extends context_reaction_menu {
  /**
   * Override of execute().
   */
  function execute() {
    if ($active_paths = $this->get_active_paths()) {
      $breadcrumb = array(l(t('Home'), '<front>', array('purl' =>array('disabled' => TRUE))));
      foreach ($active_paths as $path) {
        if ($parents = db_fetch_array(db_query("SELECT p1, p2, p3, p4, p5, p6, p7, p8 FROM {menu_links} WHERE link_path = '%s'", $path))) {
          foreach ($parents as $plid) {
            if ($parent = menu_link_load($plid)) {
              $breadcrumb[] = l($parent['title'], $parent['href']);
            }
          }
          drupal_set_breadcrumb($breadcrumb);
          break;
        }
      }
    }
  }
}

