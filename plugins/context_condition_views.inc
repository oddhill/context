<?php
// $Id$

class context_condition_views extends context_condition {

  /**
   * Generate a list of database and module provided views.
   */
  function condition_values() {
    $enabled_views = array();

    $views = views_get_all_views();

    foreach ($views as $view) {
      if (!isset($views[$view->name]->disabled) || !$views[$view->name]->disabled) {
        $enabled_views[$view->name] = $view->name;

        // Provide more granular options for each page display
        $displays = array();
        foreach ($view->display as $id => $display) {
          if ($display->display_plugin == 'page') {
            $displays[$view->name .":". $id] = "-- {$display->display_title}";
          }
        }      
        if (count($displays)) {
          $enabled_views += $displays;
        }
      }
    }
    ksort($enabled_views);
    return $enabled_views;
  }

  function execute($view) {
    switch ($view->display_handler->display->display_plugin) {
      case 'page':
      case 'calendar':
        $this->values[$view->name] = array();
        $this->values["{$view->name}:{$view->current_display}"] = array();
        // Set contexts for this view.
        $contexts = $this->get_contexts($view->name);
        foreach ($contexts as $context) {
          $this->values[$view->name][] = $context->name;
          context_set('context', $context->name, $context);
        }
        // Set any contexts associated with the current display
        if (!empty($view->current_display)) {
          $contexts = $this->get_contexts("{$view->name}:{$view->current_display}");
          foreach ($contexts as $context) {
            $this->values["{$view->name}:{$view->current_display}"][] = $context->name;
            context_set('context', $context->name, $context);
          }
        }
        break;
    }
  }
}
