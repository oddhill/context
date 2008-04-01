<?php
// $Id$

/*
 * Invokes hook_context_define() to collect all contexts provided in code by modules.
 *
 * @param $space
 *   An optional string namespace identifier. If provided, only context definitions with the
 *   specified namespace will be returned.
 *
 * @return
 *   An array of context objects.
 */
function context_ui_defaults($space = null) {
  static $contexts, $spaces;
  if (!$contexts) {
    $contexts = array();
    foreach (module_implements('context_define') as $module) {
      $function = $module .'_context_define';
      $contexts = array_merge($contexts, $function());      
    }
    foreach ($contexts as &$context) {
      $context = (object) $context;
      $spaces[$context->space][] = $context;
    }
  }
  if ($space) {
    if (isset($spaces[$space])) {
      return $spaces[$space];
    }
    else {
      return false;
    }
  }
  return $contexts;
}

/*
 * Invokes hook_context_items() to provides an array of item types that a context may be associated with.
 */
function context_ui_types($op = 'list') {
  static $types;
  if (!$types) {
    $types = array();
    foreach (module_implements('context_items') as $module) {
      $function = $module .'_context_items';
      $types = array_merge($types, $function());
    }
  }
  switch ($op) {
    case 'list':
      return array_keys($types);
      break;
    case 'full':
      return $types;
      break;
  }
}

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
      ORDER BY system ASC, space ASC, `key` ASC, value ASC");
    while ($context = db_fetch_object($result)) {
      $branch = $context->system ? 'system' : 'ui';      
      if (is_bool($context->value) || $context->value == 1 || !$context->value) {
        $tree[$branch][$context->space][$context->key] = $context;
      }
      else if (!$tree[$branch][$context->space][$context->key][$context->value]) {
        $tree[$branch][$context->space][$context->key][$context->value] = $context;
      }
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

/*
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
    $c->status = isset($ui[$c->space][$c->key][$c->value]) ? 0 : 1;
    
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
  foreach ($system as $c) {
    context_ui_context('delete', $c);
  }
}

/*
 * Page callback for context_ui admin landing page.
 */
function context_ui_admin() {
  // rebuild default contexts
  context_ui_rebuild();

  $output = '';

  // User defined contexts
  $output .= "<h3>". t('User context definitions') ."</h3>";
  $contexts = context_ui_tree('ui');
  if ($contexts) {
    $output .= theme('context_ui_admin', $contexts);
  }
  else {
    $output .= "<p>". t('Please !add_context to get started.', array('!add_context' => l(t('add a context'), 'admin/build/context/add'))) ."</p>";
  }

  // Module defined contexts
  $output .= "<h3>". t('Module context definitions') ."</h3>";  
  $contexts = context_ui_tree('system');
  if ($contexts) {
    $output .= theme('context_ui_admin', $contexts);
  }
  else {
    $output .= "<p>". t('There are currently no module defined contexts.') ."</p>";
  }
    
  return $output;
}

/*
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
            $links[] = l(t('Export'), "admin/build/context/export/$context->cid");
          }
          else {
            $links[] = l(t('Edit'), 'admin/build/context/edit/'. $context->cid);
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

/*
 * Generates the AJAX enabled block administration portion of the context_ui admin form.
 */
function theme_context_ui_block_ui($regions, $context = null) {
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

/*
 * Generates the omnibus context definition editing form.
 *
 * @param $op
 *   The type of form to build. Either "add", "view" or "edit"
 * @param $cid
 *   The db context identifier - required when $op == "edit"
 *
 * @return
 *   A Drupal form array.
 */
function context_ui_form($op, $cid = NULL) {
  drupal_add_css(drupal_get_path("module", "context_ui") ."/context_ui.css");
  drupal_add_js(drupal_get_path("module", "context_ui") ."/context_ui.js");

  switch ($op) {
    case 'view':
      if (is_numeric($cid) && $context = context_ui_context('load', $cid)) {
        drupal_set_title(t('View %title', array('%title' => $context->value)));
      }
      else {
        drupal_goto('admin/build/context'); return;
      }
      break;
    case 'edit':
      if (is_numeric($cid) && $context = context_ui_context('load', $cid)) {
        if (!$context->system) {
          drupal_set_title(t('Edit %title', array('%title' => $context->value)));
        }
        else {
          drupal_goto('admin/build/context'); return;
        }
      }
      break;
  }

  // Core context definition
  $form = array();
  $form['value'] = array(
    '#type' => 'textfield',
    '#title' => t('Value'),
    '#required' => true,
    '#maxlength' => 30,
    '#description' => t('A system name for this context. May only contain lowercase letters, underscores, and numbers. Example: <b>science_blog</b>'),
  );
  $form['key'] = array(
    '#type' => 'textfield',
    '#title' => t('Key'),
    '#default_value' => 'section',
    '#required' => true,
    '#maxlength' => 30,
    '#description' => t('The type of context information provided in this namespace. Example: <b>section</b>'),
  );
  $form['space'] = array(
    '#type' => 'textfield',
    '#title' => t('Namespace'),
    '#default_value' => 'context_ui',
    '#required' => true,
    '#maxlength' => 30,
    '#description' => t('The namespace for this context definition. Example: <b>context_ui</b>'),
  );

  $form['items'] = array(
    '#title' => t('Context Items'),
    '#tree' => true,
    '#type' => 'fieldset',
  );

  // Generate settings for context item associations
  foreach (context_ui_types('full') as $type => $item) {
      $form['items'][$type] = array(
      '#title' => $item['title'],
      '#description' => $item['description'],
      '#type' => 'checkboxes',
      '#size' => 8,
      '#multiple' => true,
      '#options' => $item['options'],
      '#prefix' => '<div class="context-ui-item clear-block">',
      '#suffix' => '</div>',
    );
  }

  // Control block visibility
  global $theme;
  $block_options =
  $block_defaults = array();
  $blocks = _context_ui_get_blocks();
  $regions = system_region_list($theme);
  // $blocks in [0] have not been assigned a region
  foreach ($blocks[0] as $block) {
    if (!isset($context->block[$block->bid])) {
      $block_options[$block->bid] = $block->label ." ($block->bid)";
    }
  }

  $form['block'] = array(
    '#type' => 'fieldset',
    '#title' => t('Blocks'),
    '#tree' => true,
    '#description' => t('Control block visibility using context. Selected blocks will be shown when this context is set provided that custom block visibility settings and/or throttling do not hide them. Grayed out blocks are those provided by Drupal\'s standard block settings. These settings apply to the current theme and any enabled themes with regions in common.'),
  );
  $form['block']['selector'] = array(
    '#type' => 'item',
    '#tree' => true,
    '#prefix' => '<div class="context-ui-block-selector">',
    '#suffix' => '</div>',
    'blocks' => array(
      '#type' => 'select',
      '#multiple' => true,
      '#size' => 12,
      '#title' => t('Blocks'),
      '#options' => $block_options,
    ),
    'regions' => array(
      '#type' => 'select',
      '#title' => t('Regions'),
      '#options' => $regions,
    ),
    'add' => array(
      '#type' => 'markup',
      '#value' => "<input id='edit-block-selector-add' class='form-submit' type='button' value='+ ". t('Add') ."'/>",
    ),
  );
  $form['block']['regions'] = array(
    '#type' => 'item',
    '#tree' => true,
    '#prefix' => '<div class="context-ui-block-regions">',
    '#suffix' => '</div>',
    '#value' => theme('context_ui_block_ui', $regions, $context),
  );
  foreach (array_keys($regions) as $region) {
    $defaults = array();
    $midpoint = false;    
    foreach (_context_ui_get_blocks($region, $context) as $block) {
      if ($block->type == 'context_ui') {
        $defaults[] = $block->bid;
      }
      else if (!$midpoint) {
        $midpoint = true;
        $defaults[] = 'system';
      }
    }
    if (!$defaults) {
      $defaults = array('system');
    }
    $defaults = implode(',', $defaults);
    $form['block']['regions'][$region] = array(
      '#type' => 'hidden',
      '#default_value' => $defaults,
    );
  }

  if ($op == 'view') {
    $form[] = array(
      '#type' => 'item',
      '#value' => l(t('Back'), 'admin/build/context'),
    );
  }
  
  if ($op != 'view') {
    $form[] = array(
      '#type' => 'submit',
      '#value' => t('Save'),    
    );
  }
  
  if ($op == 'edit') {
    $form[] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );  
  }

  if ($op == 'view' || $op == 'edit') {
    if ($context) {
      $form['value']['#default_value'] = $context->value;
      $form['key']['#default_value'] = $context->key;
      $form['space']['#default_value'] = $context->space;
      $form['cid'] = array(
        '#type' => 'value',
        '#value' => $cid,
      );
      $form['system'] = array(
        '#type' => 'value',
        '#value' => $context->system,
      );
      if ($op == 'view' || $context->system) {        
        $form['value']['#disabled'] = 
        $form['key']['#disabled'] = 
        $form['space']['#disabled'] = 
        $form['block']['selector']['blocks']['#disabled'] = 
        $form['block']['selector']['regions']['#disabled'] = true;
      }
      // Set default values for each item type (except blocks)
      foreach (context_ui_types() as $type) {
        if (is_array($context->{$type})) {
          $defaults = array();
          foreach ($context->{$type} as $id) {
            $defaults[$id] = $id;
          }
          $form['items'][$type]['#default_value'] = $defaults;
        }
        $form['items'][$type]['#disabled'] = $op == 'view' ? true : false;
      }
      // Blocks must be selected by region
      if (is_array($context->block)) {
        foreach ($regions as $region => $label) {
          if (is_array($form['block'][$region])) {
            $defaults = array();
            foreach ($form['block'][$region]['#options'] as $block => $label) {
              if (array_search($block, $context->block) !== false) {
                $defaults[$block] = $block;
              }
            }
            $form['block'][$region]['#default_value'] = $defaults;
          }
          $form['block'][$region]['#disabled'] = $op == 'view' ? true : false;
        }
      }
    }
    else {
      return drupal_goto('admin/build/context');
    }
  }  

  return $form;
}

/*
 * hook_validate()
 */
function context_ui_form_validate($form_id, $form_values) {
  $context = new stdClass();
  if ($context->value = $form_values['value']) {
    $context->space = $form_values['space'];
    $context->key = $form_values['key'];
    if (!preg_match('!^[a-z0-9_]+$!', $context->value)) {
      form_set_error('value', t('The context name can only consist of lowercase letters, underscores, and numbers.'));
    }
  }
}

/*
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
  $context->cid = $form['cid'] ? $form['cid'] : null;
  $context->system = $form['system'] ? $form['system'] : null;
  $context->space = $form['space'] ? $form['space'] : null;
  $context->key = $form['key'] ? $form['key'] : null;
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
  global $theme;
  // Get list of "valid" available blocks
  $valid = _context_ui_get_blocks();
  $valid = $valid[0];
  foreach (system_region_list($theme) as $region => $label) {
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

/*
 * hook_submit()
 */
function context_ui_form_submit($form_id, $form_values) {
  switch (t($form_values['op'])) {
    // Send user to delete confirmation page
    case 'Delete':
      return 'admin/build/context/delete/'. $form_values['cid'];
    // Process form values and save and/or update the context in the db
    case 'Save':
      $context = context_ui_form_process($form_values);
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
  return 'admin/build/context';
}

/*
 * Provides a form with an exported context definition for use in modules.
 *
 * @param $cid
 *   A context id.
 *
 * @return
 *   A FormAPI array.
 */
function context_ui_export($cid = NULL) {
  if (is_numeric($cid) && $context = context_ui_context('load', $cid)) {
    drupal_set_title(t('Export %title', array('%title' => $context->value)));

    // help text -- too bad the help module in 5 doesn't take wildcards
    $help = t('You can use exported contexts in your modules by returning an array of defined contexts in <code>hook_context_define()</code>.');

    // prune system specific information and cast for Drupal's AOP (array oriented programming)
    unset($context->cid);
    unset($context->status);
    unset($context->system);
    $context = (array) $context;

    // clean up blocks
    foreach ($context['block'] as $bid => $block) {
      unset($block->bid);
      $context['block'][$bid] = (array) $block;
    }

    // export
    $export = '$items[] = '. var_export($context, true) .';';
    
    // build the form
    $form = array();
    $form['help'] = array(
      '#type' => 'item',
      '#value' => $help,
    );
    $form['export'] = array(
      '#type' => 'textarea',
      '#rows' => 24,
      '#default_value' => $export,
    );
    return $form;
  }
  else {
    drupal_goto('admin/build/context'); return;
  }
}

/*
 * Provides simple operations (load/insert/update/etc.) on a core context space/key/value definition.
 *
 * @param $op
 *   Operation to perform on a context. May be one of load/insert/update/delete.
 * @param $context
 *   A context object. Optionally, can be an integer cid for the "load" operation.
 *
 * @return
 *   If loading, returns a full context item. All other operations return true on success and false on failure.
 */
function context_ui_context($op, $context) {
  switch ($op) {
    case 'load':
      if (is_numeric($context)) {
        $context = db_fetch_object(db_query("SELECT * FROM {context_ui} WHERE cid = %d", $context));
      }
      else if (is_object($context) && $context->cid) {
        $context = db_fetch_object(db_query("SELECT * FROM {context_ui} WHERE cid = %d", $context->cid));
      }
      else if (is_object($context) && $context->space && $context->key && $context->value) {
        $args = array(
          $context->system,
          $context->space,
          $context->key,
          $context->value
        );
        $context = db_fetch_object(db_query("SELECT * FROM {context_ui} WHERE system = %d AND space = '%s' AND `key` = '%s' AND value = '%s'", $args));
      }
      if ($context) {
        $context = context_ui_item('load', $context);
        $context = context_ui_item_block('load', $context);
        return $context;
      }
      return false;
    case 'insert':
      // check for type & existence of context definition
      $existing = context_ui_context('load', $context);      
      if (!$existing || $existing->system != $context->system) {
        $context->cid = db_next_id('{context_ui}_cid');
        $values = array(
          'cid' => $context->cid,
          'system' => $context->system,
          'status' => $context->status,
          'space' => $context->space,
          '`key`' => $context->key,
          'value' => $context->value,
        );
        $keys = implode(', ', array_keys($values));
        $args = array_merge(array($keys), $values);
        $result = db_query("INSERT INTO {context_ui} (%s) VALUES(%d, %d, %d, '%s', '%s', '%s')", $args);
        $result = $result && context_ui_item('save', $context);        
        $result = $result && context_ui_item_block('save', $context);
        return $result ? true : false;
      }
      return false;
      break;
    case 'update':
      if ($context->cid) {
        // update core context information
        $values = array(
          'system' => $context->system,
          'status' => $context->status,
          'space' => $context->space,
          '`key`' => $context->key,
          'value' => $context->value,
          'cid'   => $context->cid,
        );
        $keys = implode(', ', array_keys($values));                
        $result = db_query("UPDATE {context_ui} SET system = %d, status = %d, space = '%s', `key` = '%s', value = '%s'WHERE cid = %d", $values);
        $result = $result && context_ui_item('save', $context);
        $result = $result && context_ui_item_block('save', $context);
        return $result ? true : false;
      }
      break;
    case 'delete':
      if ($context = context_ui_context('load', $context)) {
        db_query("DELETE FROM {context_ui} WHERE cid = %d", $context->cid);
        db_query("DELETE FROM {context_ui_item} WHERE cid = %d", $context->cid);
        db_query("DELETE FROM {context_ui_block} WHERE cid = %d", $context->cid);
        return true;
      }
      return false;
  }
}

/*
 * Provides simple operations (load/save) on any context-item associations. context_ui_item() will
 * automatically sync the database with the context object provided when saving. Any associations
 * that exist on the object that are absent from the database will be inserted, and any associations
 * that are missing will be removed from the database.
 *
 * @param $op
 *   Operation to perform on a context. May be either load or save.
 * @param $context
 *   A context object with item associations.
 *
 * @return
 *   Load returns a context object with item associations. Save returns true on success and false on failure.
 */
function context_ui_item($op = 'load', $context) {
  if ($context->cid) {
    switch ($op) {
      case 'load':
        $result = db_query("SELECT * FROM {context_ui_item} WHERE cid = %d", $context->cid);
        while ($page = db_fetch_object($result)) {
          $context->{$page->type}[$page->id] = $page->id;
        }        
        return $context;
      case 'save':
        $current = (object) array('cid' => $context->cid);
        $current = context_ui_item('load', $current);
        foreach (context_ui_types() as $type) {
          // Delete any stale associations
          if (is_array($current->{$type})) {
            foreach ($current->{$type} as $id) {
              $delete = false;
              if (!is_array($context->{$type})) {
                $delete = true;
              }
              else if (array_search($id, $context->{$type}) === false) {
                $delete = true;
              }
              if ($delete) {
                $result = db_query("DELETE FROM {context_ui_item} WHERE cid = %d AND type = '%s' AND id = '%s'", $context->cid, $type, $id);
              }
            }
          }
          // Add/update any missing associations
          if (is_array($context->{$type})) {
            foreach ($context->{$type} as $id) {
              $update = false;
              if (!is_array($current->{$type})) {
                $update = true;
              }
              else if (array_search($id, $current->{$type}) === false) {
                $update = true;
              }
              if ($update) {
                $result = db_query("REPLACE INTO {context_ui_item} (cid, type, id) VALUES(%d, '%s', '%s')", $context->cid, $type, $id);            
              }
            }
          }
        }
        return true;
        break;
    }
  }
  return false;
}

/*
 * Provides simple operations (load/save) on any context-block associations. Parallel usage as
 * context_ui_item().
 *
 * @param $op
 *   Operation to perform on a context. May be either load or save.
 * @param $context
 *   A context object with an array of blocks at $context->block.
 *
 * @return
 *   Load returns a context object with block information. Save returns true on success and false on failure.
 */
function context_ui_item_block($op = 'load', $context) {
  if ($context->cid) {
    switch ($op) {
      case 'load':
        $result = db_query("SELECT module, delta, region, weight FROM {context_ui_block} WHERE cid = %d", $context->cid);
        while ($block = db_fetch_object($result)) {
          $bid = $block->module ."_". $block->delta;
          $block->bid = $bid;
          $context->block[$bid] = $block;
        }
        return $context;
        break;
      case 'save':
        // grab the current context-> block associations
        $current = (object) array('cid' => $context->cid);
        $current = context_ui_item_block('load', $current);
        // compare current definition with new definition. remove missing associations from the DB
        if (is_array($current->block)) {
          foreach ($current->block as $block) {
            if (!isset($context->block[$block->bid]) || $current->block[$block->bid] != $context->block[$block->bid]) {
              $result = db_query("DELETE FROM {context_ui_block WHERE cid = %d AND module = '%s' AND delta = '%s'", $context->cid, $block->module, $block->delta);
            }
          }
        }
        // compare new definition with current definition. add missing associations to the DB
        if (is_array($context->block)) {
          foreach ($context->block as $block) {
            $block = (object) $block;
            if (!isset($current->block[$block->bid]) || $current->block[$block->bid] != $context->block[$block->bid]) {
              $args = array(
                'module' => $block->module,
                'delta' => $block->delta,
                'region' => $block->region,
                'weight' => $block->weight,
                'cid' => $context->cid,
              );
              $result = db_query("REPLACE INTO {context_ui_block} (module, delta, region, weight, cid) VALUES ('%s', '%s', '%s', %d, %d)", $args);
            }
          }
        }
        return true;
        break;
    }    
  }
  return false;
}

/*
 * Provide a form to confirm deletion of a context definition.
 */
function context_ui_delete_confirm($cid) {
  $context = context_ui_context('load', $cid);
  if (!$context) {
    return drupal_goto('admin/build/context');
  }
  $form['cid'] = array('#type' => 'value', '#value' => $cid);
  $form = confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $context->value)),
    'admin/build/context',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel')
  );
  return $form;
}

/*
 * hook_submit()
 */
function context_ui_delete_confirm_submit($form_id, $form) {
  context_ui_context('delete', $form['cid']);
  // rebuild cache
  context_ui_rebuild();
  return 'admin/build/context';
}

/*
 * Helper function to generate a list of database and module provided views.
 */
function _context_ui_get_views() {
  $views = array();
  $result = db_query("SELECT name FROM {view_view}");
  while ($view = db_fetch_object($result)) {
    $views[$view->name] = $view->name;
  }  
  views_load_cache(); // the function below was not loaded without this call
  $default_views = _views_get_default_views();
  $views_status = variable_get('views_defaults', array());
  foreach ($default_views as $view) {
    if (!$views[$view->name] && 
      ($views_status[$view->name] == 'enabled' || (!$views_status[$view->name] && !$view->disabled))) {
      $views[$view->name] = $view->name;
    }
  }
  ksort($views);
  return $views;
}

/*
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
  global $theme;
  static $block_info, $valid, $system_blocks;
  // we don't static cache context blocks
  $context_blocks = $blocks = array();

  if (!$system_blocks) {
    // initialize regions
    foreach (system_region_list($theme) as $r => $l) {
      $system_blocks[$r] = array();      
    }
    // load blocks from database    
    $result = db_query("SELECT module, delta, weight, region, status FROM {blocks} WHERE theme = '%s'", $theme);
    while ($block = db_fetch_object($result)) {
      // load block info
      $block_info[$block->module] = $block_info[$block->module] ? $block_info[$block->module] : module_invoke($block->module, 'block', 'list');
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
  
  foreach ($blocks as &$sort_region) {
    uasort($sort_region, "_context_ui_block_compare");
  }

  return $region ? $blocks[$region] : $blocks;
}

/*
 * Helper function to sort block objects by weight
 */
function _context_ui_block_compare($a, $b) {
  // Enabled blocks
  return ($a->weight - $b->weight);
}
