<?php
// $Id$

/**
 * Context plugins.
 */
function _context_context_plugins() {
  $plugins = array();

  /**
   * Conditions.
   */
  $plugins['context_condition'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_condition.inc',
      'class' => 'context_condition',
    ),
  );
  $plugins['context_condition_node'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_condition_node.inc',
      'class' => 'context_condition_node',
      'parent' => 'context_condition',
    ),
  );
  $plugins['context_condition_sitewide'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_condition_sitewide.inc',
      'class' => 'context_condition_sitewide',
      'parent' => 'context_condition',
    ),
  );
  $plugins['context_condition_path'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_condition_path.inc',
      'class' => 'context_condition_path',
      'parent' => 'context_condition',
    ),
  );
  $plugins['context_condition_user'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_condition_user.inc',
      'class' => 'context_condition_user',
      'parent' => 'context_condition',
    ),
  );
  $plugins['context_condition_menu'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_condition_menu.inc',
      'class' => 'context_condition_menu',
      'parent' => 'context_condition',
    ),
  );
  if (module_exists('book')) {
    $plugins['context_condition_book'] = array(
      'handler' => array(
        'path' => drupal_get_path('module', 'context') .'/plugins',
        'file' => 'context_condition_book.inc',
        'class' => 'context_condition_book',
        'parent' => 'context_condition',
      ),
    );
  }
  if (module_exists('views')) {
    $plugins['context_condition_views'] = array(
      'handler' => array(
        'path' => drupal_get_path('module', 'context') .'/plugins',
        'file' => 'context_condition_views.inc',
        'class' => 'context_condition_views',
        'parent' => 'context_condition',
      ),
    );
  }

  /**
   * Reactions.
   */
  $plugins['context_reaction'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_reaction.inc',
      'class' => 'context_reaction',
    ),
  );
  $plugins['context_reaction_block'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_reaction_block.inc',
      'class' => 'context_reaction_block',
      'parent' => 'context_reaction',
    ),
  );
  $plugins['context_reaction_menu'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_reaction_menu.inc',
      'class' => 'context_reaction_menu',
      'parent' => 'context_reaction',
    ),
  );
  $plugins['context_reaction_theme'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_reaction_theme.inc',
      'class' => 'context_reaction_theme',
      'parent' => 'context_reaction',
    ),
  );
  $plugins['context_reaction_debug'] = array(
    'handler' => array(
      'path' => drupal_get_path('module', 'context') .'/plugins',
      'file' => 'context_reaction_debug.inc',
      'class' => 'context_reaction_debug',
      'parent' => 'context_reaction',
    ),
  );
  if (module_exists('css_injector')) {
    $plugins['context_reaction_css_injector'] = array(
      'handler' => array(
        'path' => drupal_get_path('module', 'context') .'/plugins',
        'file' => 'context_reaction_css_injector.inc',
        'class' => 'context_reaction_css_injector',
        'parent' => 'context_reaction',
      ),
    );
  }
  return $plugins;
}
