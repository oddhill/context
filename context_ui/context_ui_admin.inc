<?php
// $Id$

/*
 * Generates an array tree representation of available space/key/value context definitions.
 */
function context_ui_tree($op = '') {
  static $tree;
  if (!$tree) {
    $tree = array(
      'system' => array(),
      'ui' => array(),
    );
    $result = db_query("
      SELECT *
      FROM {context_ui}
      ORDER BY system ASC, space ASC, ckey ASC, value ASC");
    while ($context = db_fetch_object($result)) {
      $branch = $context->system ? 'system' : 'ui';
      $tree[$branch][$context->space][$context->ckey][$context->value] = $context;
    }
  }
  switch ($op) {
    case 'ui':
      return $tree['ui'];
    case 'system':
      return $tree['system'];
    default:
      return $tree;
  }
}

/**
 * Cache system contexts
 */
function context_ui_rebuild() {
  $default_contexts = context_ui_defaults();
  $types = context_ui_types();

  // Grab existing user defined contexts
  $ui = context_ui_tree('ui');

  // Retrieve existing system contexts
  $system = array();
  $result = db_query("SELECT * FROM {context_ui} WHERE system = 1");
  while ($existing = db_fetch_object($result)) {
    $system[$existing->cid] = true;
  }

  // Insert or update system contexts
  foreach ($default_contexts as $c) {
    // flag this context's type + status
    $c->system = 1;
    $c->status = isset($ui[$c->space][$c->ckey][$c->value]) ? 0 : 1;

    if ($context = context_ui_context('load', $c)) {
      $c->cid = $context->cid;
      context_ui_context('update', $c);
      unset($system[$c->cid]); // remove this context from unused list
    }
    else {
      context_ui_context('insert', $c);
    }
  }

  // Remove any unused contexts
  foreach ($system as $c => $dummy) {
    context_ui_context('delete', $c);
  }
}

/**
 * Generates the main context_ui admin page with a tiered context listing.
 */
function theme_context_ui_admin($context_tree) {
  foreach ($context_tree as $space => $keys) {
    $rows[] = array(
      "<span class='context-space'>". $space ."</span>",
      null
    );
    foreach ($keys as $key => $contexts) {
      if (is_array($contexts)) {
        $rows[] = array(
          "<span class='context-key'>". $key ."</span>",
          null
        );
        foreach ($contexts as $value => $context) {
          $links = array();
          $class = '';
          if ($context->system) {
            $links[] = l(t('View'), "admin/build/context/view/$context->cid");
          }
          else {
            $links[] = l(t('Edit'), 'admin/build/context/edit/'. $context->cid);
            $links[] = l(t('Export'), "admin/build/context/export/$context->cid");
            $links[] = l(t('Delete'), 'admin/build/context/delete/'. $context->cid);
          }
          if (!$context->status) {
            $class = 'overridden';
            $value = "$value (". t('Overridden') .")";
          }
          $rows[] = array(
            "<span class='context-value $class'>". $value ."</span>",
            implode(' | ', $links),
          );
        }
      }
      else {
        $links = array();
        $class = '';
        if ($contexts->system) {
          $links[] = l(t('View'), "admin/build/context/view/$contexts->cid");
        }
        else {
          $links[] = l(t('Edit'), 'admin/build/context/edit/'. $contexts->cid);
          $links[] = l(t('Delete'), 'admin/build/context/delete/'. $contexts->cid);
        }
        if ($contexts->overridden) {
          $class = 'overridden';
          $key = "$key (". t('Overridden') .")";
        }
        $rows[] = array(
          "<span class='context-key'>". $key ."</span>",
          implode(' | ', $links),
        );
      }
    }
  }
  return theme('table', array(t('Context'), t('Actions')), $rows, array('class' => 'context-ui'));
}

/**
 * Generates the AJAX enabled block administration portion of the context_ui admin form.
 */
function theme_context_ui_block_ui($regions, $context = null) {
  $output = '';

  $tools = "<div class='tools'><span class='up'></span><span class='down'></span><span class='remove'></span></div>";
  foreach ($regions as $region => $label) {
    $items = array();

    $system = _context_ui_get_blocks($region);
    $system_item = array();
    foreach ($system as $block) {
      $system_item[] = $block->label ." ($block->bid)";
    }
    $system_item = implode("<br/>", $system_item);

    $options = _context_ui_get_blocks($region, $context);
    $midpoint = false;
    if ($options) {
      foreach ($options as $block) {
        if ($block->type == 'context_ui' && !$context->system) {
          $items[] = array(
            'data' => $tools . $block->label ." ($block->bid)",
            'title' => $block->bid,
          );
        }
        else if ($block->type == 'context_ui' && $context->system) {
          $items[] = array(
            'data' => $block->label ." ($block->bid)",
            'title' => $block->bid,
            'class' => 'default',
          );
        }
        else if (!$midpoint) {
          $midpoint = true;
          $items[] = array(
            'data' => $system_item,
            'title' => 'system',
            'class' => 'disabled',
          );
        }
      }
    }
    else if ($system_item) {
      $items[] = array(
        'data' => $system_item,
        'title' => 'system',
        'class' => 'disabled',
      );
    }
    else {
      $items[] = array(
        'data' => '',
        'class' => 'dummy',
      );
    }
    $output .= theme('item_list', $items, $label, 'ul', array('class' => $region));
  }
  return $output;
}

/**
 * hook_validate()
 */
function context_ui_form_validate($form, &$form_state) {
  if ($form_state['clicked_button']['#id'] == 'edit-submit' && $form_state['values']['value']) {
    // Check for string identifier sanity
    foreach (array('value', 'ckey', 'space') as $elem) {
      if (!preg_match('!^[a-z0-9_]+$!', $form_state['values'][$elem])) {
        form_set_error($elem, t('The context !elem can only consist of lowercase letters, underscores, and numbers.', array('!elem' => $elem)));
      }
    }
    if (!isset($form_state['values']['cid'])) {
      // Check that no other user-defined context definition has taken this identifier already
      $context = new stdClass();
      $context->space = $form_state['values']['space'];
      $context->ckey = $form_state['values']['ckey'];
      $context->value = $form_state['values']['value'];
      $context->system = 0;
      if ($exists = context_ui_context('load', $context)) {
        form_set_error($form_state['values']['value'], t('A user-defined context with this space/key/value identifier already exists. Please delete the existing definition before creating a new one.'));
      }
    }
  }
}

/**
 * Produces a context object from submitted form values.
 *
 * @param $form
 *   A form array with submitted values
 *
 * @return
 *   A context object
 */
function context_ui_form_process($form) {
  $context = new stdClass();

  // Context space/key/value definition
  $context->cid = isset($form['cid']) ? $form['cid'] : null;
  $context->system = isset($form['system']) ? $form['system'] : null;
  $context->space = $form['space'] ? $form['space'] : null;
  $context->ckey = $form['ckey'] ? $form['ckey'] : null;
  $context->value = $form['value'] ? $form['value'] : null;
  $context->status = 1; // all user defined contexts have status 1

  // Values for most item types
  $item_types = context_ui_types();
  foreach ($item_types as $element_id) {
    foreach ($form['items'][$element_id] as $option => $value) {
      if ($value) {
        $context->{$element_id}[] = $option;
      }
    }
  }

  // Blocks must be done by region
  $context->block = array();
  global $theme_key;
  // Get list of "valid" available blocks
  $valid = _context_ui_get_blocks();
  $valid = $valid[0];
  foreach (system_region_list($theme_key) as $region => $label) {
    if ($blocks = $form['block']['regions'][$region]) {
      $blocks = explode(',', $blocks);
      $midpoint = array_search('system', $blocks);
      foreach ($blocks as $position => $bid) {
        if ($bid != 'system') {
          $block = $valid[$bid];
          $modifier = $position < $midpoint ? -10 : 10;
          $block->weight = $position - $midpoint + $modifier;
          $block->region = $region;
          $block->type = 'context_ui';
          $context->block[$block->bid] = $block;
        }
      }
    }
  }
  return $context;
}

/**
 * hook_submit()
 */
function context_ui_form_submit($form, &$form_state) {
  switch (t($form_state['values']['op'])) {
    // Send user to delete confirmation page
    case 'Delete':
      $form_state['redirect'] = 'admin/build/context/delete/'. $form_state['values']['cid'];
      return;
    // Process form values and save and/or update the context in the db
    case 'Save':
      $context = context_ui_form_process($form_state['values']);
      if (!$context->cid) {
        $result = context_ui_context('insert', $context);
        if ($result) {
          drupal_set_message(t('The context %title was saved successfully.', array('%title' =>  $context->value)));
          break;
        }
      }
      else if (context_ui_context('load', $context->cid)) {
        $result = context_ui_context('update', $context);
        if ($result) {
          drupal_set_message(t('The context %title was saved successfully.', array('%title' =>  $context->value)));
          break;
        }
      }
      drupal_set_message(t('An error occurred while attempting to save your context information.'), 'error');
      break;
  }
  // rebuild cache
  context_ui_rebuild();
  $form_state['redirect'] = 'admin/build/context';
}

/*
 * Helper function to generate a list of database and module provided views.
 */
function _context_ui_get_views() {
  $enabled_views = array();
  
  $views = views_get_all_views();
  
  foreach ($views as $view) {
    if (!isset($views[$view->name]->disabled) || !$views[$view->name]->disabled) {
      $enabled_views[$view->name] = $view->name;
    }
  }
  ksort($enabled_views);
  return $enabled_views;
}

/**
 * Helper function to generate a list of blocks from a specified region. If provided a context object,
 * will generate a full list of blocks for that region distinguishing between system blocks and
 * context-provided blocks.
 *
 * @param $region
 *   The string identifier for a theme region. e.g. "left"
 * @param $context
 *   A context object.
 *
 * @return
 *   A keyed (by "module_delta" convention) array of blocks.
 */
function _context_ui_get_blocks($region = null, $context = null) {
  global $theme_key;
  static $block_info, $valid, $system_blocks;
  // we don't static cache context blocks
  $context_blocks = $blocks = array();

  if (!$system_blocks) {
    // initialize regions
    foreach (system_region_list($theme_key) as $r => $l) {
      $system_blocks[$r] = array();
    }
    // load blocks from database
    $result = db_query("SELECT module, delta, weight, region, status FROM {blocks} WHERE theme = '%s' ORDER BY weight, module, delta", $theme_key);
    while ($block = db_fetch_object($result)) {
      // load block info
      $block_info[$block->module] = isset($block_info[$block->module]) ? $block_info[$block->module] : module_invoke($block->module, 'block', 'list');
      $block->label = $block_info[$block->module][$block->delta]['info'];
      $block->type = 'system';
      $block->bid = $block->module .'_'. $block->delta;
      // add block to region
      if ($block->region && $block->status) {
        $system_blocks[$block->region][$block->bid] = $block;
      }
      else {
        $system_blocks[0][$block->bid] = $block;
      }
      // mark block as available in DB
      $valid[$block->module ."_". $block->delta] = true;
    }
  }

  // load system blocks into main block array
  $blocks = $system_blocks;

  // load context blocks if provided
  if (is_object($context) && is_array($context->block)) {
    // iterate over context-associated blocks
    foreach ($context->block as $block) {
      $block = (object) $block;
      // check that this is a valid block
      if ($valid[$block->module ."_". $block->delta]) {
        // if region has been specified, ensure that block belongs to it
        if (!$region || (isset($region) && $block->region == $region)) {
          // load block info
          $block_info[$block->module] = $block_info[$block->module] ? $block_info[$block->module] : module_invoke($block->module, 'block', 'list');
          $block->label = $block_info[$block->module][$block->delta]['info'];
          $block->type = 'context_ui';
          $block->bid = $block->module .'_'. $block->delta;
          // add block to region
          if ($block->region) {
            $blocks[$block->region][$block->bid] = $block;
          }
          else {
            $blocks[0][$block->bid] = $block;
          }
        }
      }
    }
  }

  foreach ($blocks as $r => &$sort_region) {
    if ($r !== 0) {
      uasort($sort_region, "_context_ui_block_compare");
    }
  }

  return $region ? $blocks[$region] : $blocks;
}
