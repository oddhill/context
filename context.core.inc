<?php

/**
 * Implementation of hook_theme().
 */
function context_theme() {
  $items = array();
  $items['context_links'] = array(
    'arguments' => array(),
  );
  return $items;
}

/**
 * Implementation of hook_theme_registry_alter().
 */
function context_theme_registry_alter(&$theme_registry) {
  // Push theme_page() through a context_preprocess to provide
  // context-sensitive menus and variables.
  if (!in_array('context_preprocess_page', $theme_registry['page']['preprocess functions'])) {
    $theme_registry['page']['preprocess functions'][] = 'context_preprocess_page';
  }
  // Reroute theme_blocks() through context_blocks to determine block
  // visibility by context.
  unset($theme_registry['blocks']['preprocess functions']);
  $theme_registry['blocks']['function'] = 'context_blocks';
}

/**
 * Implementation of hook_context_conditions().
 *
 * Allows modules to integrate with context and provide their native
 * objects as options for setting a context definition. The
 * hook should return an array of items keyed on the object "type"
 * (e.g. "node", "user", etc.) with key-value pairs corresponding to
 * a FormAPI element array with some restrictions and additional info.
 *
 * '#title': Required. The title of the object / form option.
 * '#type': Required. The FormAPI element type to use. Currently only
 *   'select', 'checkboxes', 'radio', and 'textfield' are allowed.
 * '#description': Optional. Help text to be displayed on the form.
 * '#options': Required. A key-value array of options. They key will be
 *   stored and passed to context_set_by_condition(), so the integrating module
 *   should use a unique (within its namespace) / usable identifier.
 */
function context_context_conditions() {
  $items = array();

  // Content Types
  $nodetypes = array();
  foreach (node_get_types() as $type) {
    $nodetypes[$type->type] = t(drupal_ucfirst($type->name));
  }
  $items['node'] = array(
    '#title' => t('Content Types'),
    '#description' => t('Set this context when viewing a node page or using the add/edit form of one of these content types.'),
    '#options' => $nodetypes,
    '#type' => 'checkboxes',
  );

  // User
  $items['user'] = array(
    '#title' => t('User Pages'),
    '#description' => t('Set this context when a user with selected role(s) is viewed'),
    '#options' => user_roles(true),
    '#type' => 'checkboxes',
  );

  // Book
  if (module_exists('book')) {
    $options = array();
    foreach(book_get_books() as $book) {
      $options[$book['menu_name']] = $book['title'];
    }
    $items['book'] = array(
      '#title' => t('Book'),
      '#description' => t('Set this context when a node in the selected book is viewed.'),
      '#options' => $options,
      '#type' => 'checkboxes',
    );
  }

  // Sitewide context
  $items['sitewide'] = array(
    '#title' => t('Sitewide context'),
    '#type' => 'radios',
    '#options' => array(0 => t('False'), 1 => t('True')),
    '#description' => t('Should this context always be set? If <strong>true</strong>, this context will be active across your entire site.'),
  );

  // Path
  $items['path'] = array(
    '#title' => t('Path'),
    '#description' => t('Set this context when any of the paths above match the beginning or all of the page path. Put each path on a separate line.'),
    '#type' => 'textarea',
    '#element_validate' => array('context_condition_text_validate'),
  );

  return $items;
}

/**
 * Element validate handler for setter textareas and texfields.
 * Will process and convert a string to an array of matchable
 * elements by splitting on an appropriate delimiter ("\n" for
 * textareas and "," for textfields).
 */
function context_condition_text_validate($element, &$form_state) {
  if (!empty($element['#value']) && in_array($element['#type'], array('textfield', 'textarea'))) {
    switch ($element['#type']) {
      case 'textfield':
        $delimiter = ',';
        break;
      case 'textarea':
        $delimiter = "\n";
        break;
    }

    $items = $element['#value'];
    $items = explode($delimiter, $items);

    if (!empty($items)) {
      $values = array();
      foreach ($items as $k => $v) {
        $v = trim($v);
        if (!empty($v)) {
          $values[$v] = TRUE;
        }
      }
      $id = end($element['#parents']);
      $form_state['values']['items'][$id] = $values;
    }
  }
}

/**
 * Implementation of hook_context_reactions().
 *
 * Allows modules to integrate with context and provide options for
 * responding when a context has been set. The hook should return an
 * array of items keyed on the "type" of getter (e.g. "menu", "theme",
 * etc.) with key-value pairs corresponding to a FormAPI element array
 * with some restrictions and additional info.
 *
 * The getter element array provided differs from the setter array in
 * that it may store a tree of values (i.e. where #tree => true). The
 * values will be stored in a serialized array in the database.
 *
 * '#title': Required. The title of the object / form option.
 * '#type': Required. The FormAPI element type to use. Currently only
 *   'select', 'checkboxes', 'radio', and 'textfield' are allowed.
 * '#description': Optional. Help text to be displayed on the form.
 * '#options': Required. A key-value array of options. They key will be
 *   stored and passed to context_set_by_condition(), so the integrating module
 *   should use a unique (within its namespace) / usable identifier.
 */
function context_context_reactions() {
  $items = array();

  // Menu
  if (module_exists('menu')) {
    $menus = menu_parent_options(array_reverse(menu_get_menus()), NULL);
    foreach ($menus as $key => $name) {
      $id = explode(':', $key);
      // @TODO: choose a good convention for excluding root menus from
      // being selectable items
      if ($id[1] == '0') {
        // this is required because root menu names are displayed inside <> and will not be displayed otherwise
        $menus['_'. $key] = '<strong>'. check_plain($name) .'</strong>';
      }
      else {
        $link = menu_link_load($id[1]);
        $menus[$link['link_path']] = $name;
      }
      unset($menus[$key]);
    }
    array_unshift($menus, '<strong>&lt;'. t('None') .'&gt;</strong>');
    $items['menu'] = array(
      '#title' => t('Menus'),
      '#description' => t('Display the selected menu item as active when this context is set. To use this feature, you must use <strong>theme_context_links()</strong> to theme your links. Please see README.txt for more information.'),
      '#options' => $menus,
      '#type' => 'radios',
    );
  }

  // Implements context-based theme improvements
  $items['theme_section'] = array(
    '#tree' => true,
    '#title' => t('Section info'),
    'title' => array(
      '#title' => t('Section title'),
      '#description' => t('Provides this text as a <strong>$section_title</strong> variable for display in page.tpl.php when this context is active.'),
      '#type' => 'textfield',
      '#maxlength' => 255,
    ),
    'subtitle' => array(
      '#title' => t('Section subtitle'),
      '#description' => t('Provides this text as a <strong>$section_subtitle</strong> variable for display in page.tpl.php when this context is active.'),
      '#type' => 'textfield',
      '#maxlength' => 255,
    ),
    'class' => array(
      '#title' => t('Section class'),
      '#description' => t('Provides this text as an additional body class (in <strong>$body_classes</strong> in page.tpl.php) when this section is active. Note that there may only be <strong>one</strong> active section class at once.'),
      '#type' => 'textfield',
      '#maxlength' => 64,
    ),
  );

  // Implements context-based region disabling
  $theme_key = variable_get('theme_default', 'garland');
  $regions = system_region_list($theme_key);
  $items['theme_regiontoggle'] = array(
    '#title' => t('Disabled regions'),
    '#type' => 'checkboxes',
    '#options' => $regions,
  );

  return $items;
}


/**
 * Implementation of hook_nodeapi().
 */
function context_nodeapi(&$node, $op, $teaser, $page) {
  if ($op == 'view' && $page && menu_get_object() === $node) {
    // Implementation of context_set_by_condition for node.
    context_set_by_condition('node', $node->type);

    // Implementation of context_set_by_condition for book.
    if (module_exists('book') && isset($node->book)) {
      if ($node->book['menu_name']) {
        context_set_by_condition('book', $node->book['menu_name']);
      }
    }
  }
}

/**
 * Implementation of hook_form_alter().
 */
function context_form_alter(&$form, $form_state, $form_id) {
  if (isset($form['#node']) && arg(0) != 'admin') { // Prevent this from firing on admin pages... damn form driven apis...
    context_set_by_condition('node', $form['#node']->type);
  }
  if ($form_id == 'system_modules') {
    context_invalidate_cache();
  }
}

/**
 * Implementation of hook_form_alter() for comment_form.
 */
function context_form_comment_form_alter(&$form, $form_state) {
  if ($nid = $form['nid']['#value']) {
    $node = node_load($nid);
    context_set_by_condition('node', $node->type);
  }
}

/**
 * Implementation of hook_user().
 */
function context_user($op, &$edit, &$account, $category = NULL) {
  if ($op == 'view' && !empty($account->roles)) {
    foreach (array_keys($account->roles) as $rid) {
      context_set_by_condition('user', $rid);
    }
  }
}

/**
 * BLOCK HANDLING =====================================================
 */

/**
 * This override of theme_blocks() is called because of an alter of the
 * theme registry. See context_theme_registry_alter().
 */
function context_blocks($region) {
  $output = "";
  if ($list = context_block_list($region)) {
    foreach ($list as $key => $block) {
      $output .= theme("block", $block);
    }
  }
  // Add any content assigned to this region through drupal_set_content() calls.
  $output .= drupal_get_content($region);
  return $output;
}

/**
 * An alternative version of block_list() that provides any context enabled blocks.
 */
function context_block_list($region) {
  static $blocks;
  static $context_blocks;
  static $disabled_regions;

  if (!isset($context_blocks)) {
    $blocks = array();
    $context_blocks = array();
    $disabled_regions = array();

    // Store all active context blocks when first called
    foreach (context_active_contexts() as $context) {
      if (isset($context->block) && is_array($context->block)) {
        foreach ($context->block as $block) {
          $block = (object) $block;
          $context_blocks["{$block->module}_{$block->delta}"] = $block;
        }
      }
      if (!empty($context->theme_regiontoggle)) {
        foreach ($context->theme_regiontoggle as $r => $enabled) {
          if (!empty($enabled)) {
            $disabled_regions[$r] = $r;
          }
        }
      }
    }

    global $user, $theme_key;
    $rids = array_keys($user->roles);

    // This query is identical to the one in block_list(), but status = 1 is excluded to
    // retain blocks that may be enabled via context.
    $result = db_query(db_rewrite_sql("SELECT DISTINCT b.* FROM {blocks} b LEFT JOIN {blocks_roles} r ON b.module = r.module AND b.delta = r.delta WHERE b.theme = '%s' AND (r.rid IN (". db_placeholders($rids) .") OR r.rid IS NULL) ORDER BY b.region, b.weight, b.module", 'b', 'bid'), array_merge(array($theme_key), $rids));

    while ($block = db_fetch_object($result)) {
      $bid = "{$block->module}_{$block->delta}";

      // If block is not enabled & not enabled via context, skip it
      if (!empty($context_blocks[$bid])) {
        $block->region = $context_blocks[$bid]->region;
        $block->weight = $context_blocks[$bid]->weight;
        $enabled = TRUE;
      }
      else if (!$block->status) {
        continue;
      }

      // Initialize region key
      if (!isset($blocks[$block->region])) {
        $blocks[$block->region] = array();
      }

      // Use the user's block visibility setting, if necessary
      if ($block->custom != 0) {
        if ($user->uid && isset($user->block[$block->module][$block->delta])) {
          $enabled = $user->block[$block->module][$block->delta];
        }
        else {
          $enabled = ($block->custom == 1);
        }
      }
      else {
        $enabled = TRUE;
      }

      // Match path if necessary
      if ($block->pages) {
        if ($block->visibility < 2) {
          $path = drupal_get_path_alias($_GET['q']);
          // Compare with the internal and path alias (if any).
          $page_match = drupal_match_path($path, $block->pages);
          if ($path != $_GET['q']) {
            $page_match = $page_match || drupal_match_path($_GET['q'], $block->pages);
          }
          // When $block->visibility has a value of 0, the block is displayed on
          // all pages except those listed in $block->pages. When set to 1, it
          // is displayed only on those pages listed in $block->pages.
          $page_match = !($block->visibility xor $page_match);
        }
        else {
          $page_match = drupal_eval($block->pages);
        }
      }
      else {
        $page_match = TRUE;
      }

      $block->enabled = $enabled;
      $block->page_match = $page_match;
      $blocks[$block->region]["{$block->module}_{$block->delta}"] = $block;
    }

    // Sort blocks -- we must do this here since blocks provided via
    // context may have overridden or altered weights.
    foreach ($blocks as $r => $dummy) {
      uasort($blocks[$r], '_context_block_sort');
    }
  }

  // Kill blocks in disabled regions
  foreach (array_keys($disabled_regions) as $r) {
    unset($blocks[$r]);
  }

  // ==================================================================
  // The block rendering code below is identical to block_list().
  // ==================================================================

  // Create an empty array if there were no entries
  if (!isset($blocks[$region])) {
    $blocks[$region] = array();
  }

  foreach ($blocks[$region] as $key => $block) {
    // Render the block content if it has not been created already.
    if (!isset($block->content)) {
      // Erase the block from the static array - we'll put it back if it has content.
      unset($blocks[$region][$key]);
      if ($block->enabled && $block->page_match) {
        // Check the current throttle status and see if block should be displayed
        // based on server load.
        if (!($block->throttle && (module_invoke('throttle', 'status') > 0))) {
          // Try fetching the block from cache. Block caching is not compatible with
          // node_access modules. We also preserve the submission of forms in blocks,
          // by fetching from cache only if the request method is 'GET'.
          if (!count(module_implements('node_grants')) && $_SERVER['REQUEST_METHOD'] == 'GET' && ($cid = _block_get_cache_id($block)) && ($cache = cache_get($cid, 'cache_block'))) {
            $array = $cache->data;
          }
          else {
            $array = module_invoke($block->module, 'block', 'view', $block->delta);
            if (isset($cid)) {
              cache_set($cid, $array, 'cache_block', CACHE_TEMPORARY);
            }
          }

          if (isset($array) && is_array($array)) {
            foreach ($array as $k => $v) {
              $block->$k = $v;
            }
          }
        }
        if (isset($block->content) && $block->content) {
          // Override default block title if a custom display title is present.
          if ($block->title) {
            // Check plain here to allow module generated titles to keep any markup.
            $block->subject = $block->title == '<none>' ? '' : check_plain($block->title);
          }
          if (!isset($block->subject)) {
            $block->subject = '';
          }
          $blocks[$block->region]["{$block->module}_{$block->delta}"] = $block;
        }
      }
    }
  }

  return $blocks[$region];
}

/**
 * Helper function to sort blocks.
 */
function _context_block_sort($a, $b) {
  return ($a->weight - $b->weight);
}

/**
 * THEME FUNCTIONS & RELATED ==========================================
 */

/**
 * Generates a themed set of links for node types associated with
 * the current active contexts.
 */
function theme_context_links($links) {
  $output = '';
  foreach ($links as $link) {
    $options = array_merge($link, array('attributes' => array('class' => 'button')));
    if (!empty($link['custom'])) {
      $output .= l($link['title'], $link['href'], $options);
    }
    else {
      $output .= l('+ '. t('Add !type', array('!type' => $link['title'])), $link['href'], $options);
    }
  }
  return $output;
}

/**
 * Generates an array of links (suitable for use with theme_links)
 * to the node forms of types associated with current active contexts.
 */
function context_links($reset = false) {
  static $links;
  if (!$links || $reset) {
    $links = array();
    if ($contexts = context_active_contexts()) {
      // Collect types
      $types = node_get_types();
      // Iterate over active contexts
      foreach ($contexts as $context) {
        if (!empty($context->node)) {
          foreach ($context->node as $type) {
            $type_url = str_replace('_', '-', $type);
            $add_url = 'node/add/'. $type_url;
            if (isset($types[$type]) && strpos($_GET['q'], $add_url) === FALSE && node_access('create', $type)) {
              $links[$type_url] = array('title' => $types[$type]->name, 'href' => $add_url);
            }
          }
        }
      }
    }
    drupal_alter('context_links', $links);
  }
  return $links;
}

/**
 * Implementation of preprocess_page().
 */
function context_preprocess_page(&$vars) {
  $info = context_section_info();
  $vars['section_title'] = !empty($info['title']) ? $info['title'] : '';
  $vars['section_subtitle'] = !empty($info['subtitle']) ? $info['subtitle'] : '';
  $vars['body_classes'] .= !empty($info['class']) ? ' '. $info['class'] : '';

  // If primary + secondary links are pointed at the same menu, provide
  // contextual trailing by default.
  if (variable_get('menu_primary_links_source', 'primary-links') == variable_get('menu_secondary_links_source', 'secondary-links')) {
    $vars['primary_links'] = context_menu_navigation_links(variable_get('menu_primary_links_source', 'primary-links'));
    $vars['secondary_links'] = context_menu_navigation_links(variable_get('menu_secondary_links_source', 'secondary-links'), 1);
  }

  $vars['primary_links'] = context_menu_set_active($vars['primary_links']);
  $vars['secondary_links'] = context_menu_set_active($vars['secondary_links']);

  if ($context_links = context_links()) {
    $vars['context_links'] = theme('context_links', $context_links);
  }
}

/**
 * Iterates through a provided links array for use with theme_links()
 * (e.g. from menu_primary_links()) and provides an active class for
 * any items that have a path that matches an active context.
 *
 * @param $links
 *   An array of links.
 * @param $reset
 *   A boolean flag for resetting the static cache.
 *
 * @return
 *   A modified links array.
 */
function context_menu_set_active($links = array(), $reset = FALSE) {
  // Build a list of active paths & static cache it
  static $active_paths;
  if (!isset($active_paths) || $reset) {
    $active_paths = array();
    foreach (context_active_contexts() as $context) {
      if (isset($context->menu)) {
        $active_paths[] = $context->menu;
      }
    }
  }

  // Iterate through the provided links and build a new set of links
  // that includes active classes
  $new_links = array();
  if (!empty($links)) {
    foreach ($links as $key => $link) {
      if (!empty($link['href']) && in_array($link['href'], $active_paths)) {
        if (isset($links['attributes'])) {
          if (isset($links['attributes']['class'])) {
            $link['attributes']['class'] .= ' active';
          }
          else {
            $link['attributes']['class'] = 'active';
          }
        }
        else {
          $link['attributes'] = array('class' => 'active');
        }
        if (strpos(' active', $key) === FALSE) {
          $new_links[$key .' active'] = $link;
        }
      }
      else {
        $new_links[$key] = $link;
      }
    }
  }

  return $new_links;
}

/**
 * Retrieves an array of current section information and static caches it.
 *
 * @param $reset
 *   A boolean flag to reset the static cache.
 *
 * @return
 *   A section title string.
 */
function context_section_info($reset = FALSE) {
  static $info;
  if (!isset($info) || $reset) {
    $info = array();
    foreach (context_active_contexts() as $context) {
      if (isset($context->theme_section)) {
        if (!empty($context->theme_section['title']) && !isset($info['title'])) {
          $info['title'] = $context->theme_section['title'];
        }
        if (!empty($context->theme_section['subtitle']) && !isset($info['subtitle'])) {
          $info['subtitle'] = $context->theme_section['subtitle'];
        }
        if (!empty($context->theme_section['class'])) {
          if (!isset($info['class'])) {
            $info['class'] = $context->theme_section['class'];
          }
          else {
            $info['class'] .= ' '. $context->theme_section['class'];
          }
        }
      }
    }
  }
  return $info;
}

/**
 * Wrapper around menu_navigation_links() that gives themers the option of
 * building navigation links based on an active context trail.
 */
function context_menu_navigation_links($menu_name, $level = 0) {
  // Retrieve original path so we can repair it after our hack.
  $original_path = $_GET['q'];

  // Retrieve the first active menu path found.
  foreach (context_active_contexts() as $context) {
    if (isset($context->menu) && !empty($context->menu) && menu_get_item($context->menu)) {
      menu_set_active_item($context->menu);
      break;
    }
  }

  // Build the links requested
  $links = menu_navigation_links($menu_name, $level);

  // Repair and get out
  menu_set_active_item($original_path);
  return $links;
}
