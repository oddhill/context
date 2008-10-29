<?php
// $Id$
/**
 * Page callback for context_ui admin landing page.
 */
function context_ui_admin() {
  // Add css
  drupal_add_css(drupal_get_path("module", "context_ui") ."/context_ui.css");

  // rebuild blocks
  _block_rehash();

  // rebuild default contexts
  context_ui_rebuild();

  // User defined contexts
  $output = "<h3>". t('User context definitions') ."</h3>";
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

/**
 * Generates the omnibus context definition editing form.
 * Note: submission and validation handlers are in context_ui_admin.inc
 *
 * @param $op
 *   The type of form to build. Either "add", "view" or "edit"
 * @param $cid
 *   The db context identifier - required when $op == "edit"
 *
 * @return
 *   A Drupal form array.
 */
function context_ui_form(&$form_state, $op, $cid = NULL, $context = NULL) {
  drupal_add_css(drupal_get_path("module", "context_ui") ."/context_ui.css");
  drupal_add_js(drupal_get_path("module", "context_ui") ."/context_ui.js");

  switch ($op) {
    case 'add':
      if (is_object($context)) {
        $context->system = true;
        if ($exists = context_ui_context('load', $context)) {
          drupal_set_message(t('A module is already providing a context with this space/key/value identifier. Your context definition will override its settings.'));
        }
        $context->system = false;
      }
      break;
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
          drupal_set_title(t('Edit context: %title', array('%title' => $context->value)));
        }
        else {
          drupal_goto('admin/build/context'); return;
        }
      }
      break;
    case 'clone':
      if (is_numeric($cid) && $context = context_ui_context('load', $cid)) {
        drupal_set_title(t('Clone context: %title', array('%title' => $context->value)));
        $context->system = 0;
        $context->cid = null;
        $cid = null;
      }
      else {
        drupal_goto('admin/build/context'); return;
      }
      break;
  }

  // Core context definition
  $form = array(
    '#base' => 'context_ui_form',
    '#theme' => 'context_ui_form',
  );

  $form['value'] =
  $form['attribute'] =
  $form['namespace'] = array(
    '#type' => 'textfield',
    '#required' => true,
    '#maxlength' => 64,
    '#size' => 20,
  );

  $form['value']['#title'] = t('Value');
  $form['value']['#description'] = t('A system name for this context. May only contain lowercase letters, underscores, and numbers. Example: <b>science_blog</b>');

  $form['attribute']['#title'] = t('Attribute');
  $form['attribute']['#default_value'] = 'section';
  $form['attribute']['#description'] = t('The type of context information provided in this namespace. Example: <b>section</b>');

  $form['namespace']['#title'] = t('Namespace');
  $form['namespace']['#default_value'] = 'context_ui';
  $form['namespace']['#description'] = t('The namespace for this context definition. Example: <b>context_ui</b>');

  $form['items'] = array(
    '#tree' => true,
  );

  // Generate settings for context item associations
  foreach (context_ui_types('full') as $type => $item) {
    if (in_array($item['#type'], array('select', 'radios', 'checkboxes', 'textfield'))) {
      $form['items'][$type] = $item;
    }
  }

  // Control block visibility
  init_theme(); // we need to initialize theme in order to deal with blocks
  global $theme_key;
  $block_options =
  $block_defaults = array();
  $blocks = _context_ui_get_blocks();
  $regions = system_region_list($theme_key);
  // $blocks in [0] have not been assigned a region
  foreach ($blocks[0] as $block) {
    if (!isset($context->block[$block->bid])) {
      $block_options[$block->module][$block->bid] = $block->label ." ($block->bid)";
    }
  }
  ksort($block_options);

  $form['block'] = array(
    '#tree' => true,
  );

  $form['block']['selector'] = array(
    '#description' => t('Control block visibility using context. Selected blocks will be shown when this context is set provided that custom block visibility settings and/or throttling do not hide them. Grayed out blocks are those provided by Drupal\'s standard block settings. These settings apply to the current theme and any enabled themes with regions in common.'),
    '#type' => 'item',
    '#tree' => true,
    '#prefix' => '<div class="context-ui-block-selector">',
    '#suffix' => '</div>',
    'blocks' => array(
      '#type' => 'select',
      '#multiple' => true,
      '#size' => 15,
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
    $form['back'] = array(
      '#type' => 'item',
      '#value' => l(t('Back'), 'admin/build/context'),
    );
  }

  if ($op != 'view') {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
  }

  if ($op == 'edit') {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }

  if ($op == 'view' || $op == 'edit' || $op == 'clone' || ($op == 'add' && $context)) {
    if ($context) {
      $form['value']['#default_value'] = $context->value;
      $form['attribute']['#default_value'] = $context->attribute;
      $form['namespace']['#default_value'] = $context->namespace;
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
        $form['attribute']['#disabled'] =
        $form['namespace']['#disabled'] =
        $form['block']['selector']['blocks']['#disabled'] =
        $form['block']['selector']['regions']['#disabled'] = true;
      }
      // Set default values for each item type (except blocks)
      foreach (context_ui_types('full') as $type => $item) {
        if (isset($context->{$type}) && is_array($context->{$type})) {
          if ($item['#type'] == 'checkboxes' || ($item['#type'] == 'select' && $item['#multiple'] == true)) {
            $defaults = array();
            foreach ($context->{$type} as $id) {
              $defaults[$id] = $id;
            }
          }
          else {
            $defaults = current($context->{$type});
          }
          $form['items'][$type]['#default_value'] = $defaults;
        }
        $form['items'][$type]['#disabled'] = $op == 'view' ? true : false;
      }
      // Blocks must be selected by region
      if (is_array($context->block)) {
        foreach ($regions as $region => $label) {
          if (isset($form['block'][$region]) && is_array($form['block'][$region])) {
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

/**
 * Theme function for context_ui_form()
 */
function theme_context_ui_form($form) {
  $output = '';

  // Render space / key / value trio in a 3-column table
  $header = array(t('Namespace'), t('Attribute'), t('Value'));
  unset($form['namespace']['#title']);
  unset($form['attribute']['#title']);
  unset($form['value']['#title']);
  $rows = array(
    array(
      drupal_render($form['namespace']),
      drupal_render($form['attribute']),
      drupal_render($form['value']),
    ),
  );
  $output .= theme('table', $header, $rows, array('class' => 'context-ui-3col'));

  // Render setters / getters as a two column split
  $header = array(t('Set context'), t('Respond to context'));
  $setters = $getters = '';
  foreach (context_ui_types('full') as $type => $item) {
    if ($item['#context_ui'] == 'getter') {
      $getters .= drupal_render($form['items'][$type]);
    }
    else {
      $setters .= drupal_render($form['items'][$type]);
    }
  }
  $rows = array(
    array(
      array('data' => $setters, 'class' => 'setters left'),
      array('data' => $getters, 'class' => 'getters right'),
    ),
  );
  $output .= theme('table', $header, $rows, array('class' => 'context-ui-2col'));

  // Block visibility
  $header = array(t('Blocks'), t('Regions'));
  $rows = array(
    array(
      array('data' => drupal_render($form['block']['selector']), 'class' => 'left'),
      array('data' => drupal_render($form['block']['regions']), 'class' => 'right'),
    ),
  );
  $output .= theme('table', $header, $rows, array('id' => 'context-ui-blocks', 'class' => 'context-ui-2col'));

  $output .= drupal_render($form);
  return $output;
}

/**
 * Provide a form to confirm deletion of a context definition.
 */
function context_ui_delete_confirm(&$form_state, $cid) {
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

/**
 * Submit hook for context_ui delete confirmation form.
 */
function context_ui_delete_confirm_submit($form, &$form_state) {
  context_ui_context('delete', $form_state['values']['cid']);
  context_ui_rebuild();
  $form_state['redirect'] = 'admin/build/context';
}

/**
 * Page callback for import form. Switches form output to context form
 * if import submission has occurred.
 */
function context_ui_import_page() {
  if (!empty($_POST) && $_POST['form_id'] == 'context_ui_form') {
    return drupal_get_form('context_ui_form', 'add');
  }
  return drupal_get_form('context_ui_import');
}

/**
 * Import form. Provides simple helptext instructions and textarea for
 * pasting a context definition.
 */
function context_ui_import() {
  drupal_set_title(t('Import context'));
  $help = t('You can import a context definition by pasting the exported context object code into the field below.');
  $form = array();
  $form['help'] = array(
    '#type' => 'item',
    '#value' => $help,
  );
  $form['import'] = array(
    '#title' => t('Context Object'),
    '#type' => 'textarea',
    '#rows' => 10,
    '#required' => true,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );

  return $form;
}

/**
 * Import form submit handler. Evaluates import code and transfers to
 * context definition form.
 */
function context_ui_import_submit($form, &$form_state) {
  $items = array();
  if ($import = $form_state['values']['import']) {
    ob_start();
    eval($import);
    ob_end_clean();
  }
  if (is_array($items) && count($items)) {
    $context = current($items);
  }
  if (is_array($context)) {
    $context = (object) $context;
    $context->system = FALSE;
    if ($exists = context_ui_context('load', $context)) {
      drupal_set_message(t('A user-defined context definition with this space/key/value identifier already exists. Please remove the existing context before importing this definition.'), 'error');
      $form_state['redirect'] = 'admin/build/context';
    }
    else {
      drupal_set_title(t('Add context'));
      $output = drupal_get_form('context_ui_form', 'add', null, (object) $context);
      print theme('page', $output);
      exit;
    }
  }
  else {
    drupal_set_message(t('An error occurred while importing. Please check your context definition.', 'error'));
    $form_state['redirect'] = 'admin/build/context';
  }
}

/**
 * Provides a form with an exported context definition for use in modules.
 *
 * @param $cid
 *   A context id.
 *
 * @return
 *   A FormAPI array.
 */
function context_ui_export(&$form_state, $cid = NULL) {
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

/**
 * Generates an array tree representation of available space/key/value context definitions.
 */
function context_ui_tree($op = '', $reset = FALSE) {
  static $tree;
  if (!$tree || $reset) {
    $tree = array(
      'system' => array(),
      'ui' => array(),
    );
    $result = db_query("
      SELECT *
      FROM {context_ui}
      ORDER BY system ASC, namespace ASC, attribute ASC, value ASC");
    while ($context = db_fetch_object($result)) {
      $branch = $context->system ? 'system' : 'ui';
      $tree[$branch][$context->namespace][$context->attribute][$context->value] = $context;
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
  // Always reset this static cache -- since this could be called
  // before a user-defined context is inserted on submit.
  $ui = context_ui_tree('ui', TRUE);

  // Retrieve existing system contexts
  $system = array();
  $result = db_query("SELECT * FROM {context_ui} WHERE system = 1");
  while ($existing = db_fetch_object($result)) {
    $system[$existing->cid] = true;
  }

  // Insert or update system contexts
  foreach ($default_contexts as $c) {
    // Check that the providing module has not explicitly set system flag.
    // If it has, we trust that the module knows what it is doing.
    if (!isset($c->system)) {
      $c->system = 1;
    }

    // Check that the context definition is reasonable
    if (!empty($c->namespace) && !empty($c->attribute)) {
      $context = context_ui_context('load', $c);

      // Check that the providing module has not explicitly set the status.
      if (!isset($c->status)) {
        $c->status = isset($ui[$c->namespace][$c->attribute][$c->value]) ? 0 : 1;
      }

      if ($context) {
        $c->cid = $context->cid;
        context_ui_context('update', $c);
        unset($system[$c->cid]); // remove this context from unused list
      }
      else {
        context_ui_context('insert', $c);
      }
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
  foreach ($context_tree as $namespace => $attributes) {
    $rows[] = array(
      "<span class='context-namespace'>". $namespace ."</span>",
      null
    );
    foreach ($attributes as $attribute => $contexts) {
      if (is_array($contexts)) {
        $rows[] = array(
          "<span class='context-attribute'>". $attribute ."</span>",
          null
        );
        foreach ($contexts as $value => $context) {
          $links = array();
          $class = '';
          if ($context->system) {
            $links[] = l(t('View'), "admin/build/context/view/$context->cid");
            $links[] = l(t('Override'), 'admin/build/context/clone/'. $context->cid);
          }
          else {
            $links[] = l(t('Edit'), 'admin/build/context/edit/'. $context->cid);
            $links[] = l(t('Export'), "admin/build/context/export/$context->cid");
            $links[] = l(t('Clone'), 'admin/build/context/clone/'. $context->cid);
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
    foreach (array('value', 'attribute', 'namespace') as $elem) {
      if (!preg_match('!^[a-z0-9_]+$!', $form_state['values'][$elem])) {
        form_set_error($elem, t('The context !elem can only consist of lowercase letters, underscores, and numbers.', array('!elem' => $elem)));
      }
    }
    if (!isset($form_state['values']['cid'])) {
      // Check that no other user-defined context definition has taken this identifier already
      $context = new StdClass();
      $context->namespace = $form_state['values']['namespace'];
      $context->attribute = $form_state['values']['attribute'];
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
  
  // Context ns/attr/value definition
  $context->cid = isset($form['cid']) ? $form['cid'] : null;
  $context->system = isset($form['system']) ? $form['system'] : null;
  $context->namespace = isset($form['namespace']) ? $form['namespace'] : null;
  $context->attribute = isset($form['attribute']) ? $form['attribute'] : null;
  $context->value = isset($form['value']) ? $form['value'] : null;
  $context->status = 1; // all user defined contexts have status 1

  // Values for most item types
  $item_types = context_ui_types();
  foreach ($item_types as $element_id) {
    if (is_array($form['items'][$element_id])) {
      foreach ($form['items'][$element_id] as $option => $value) {
        if ($value) {
          $context->{$element_id}[] = $option;
        }
      }
    }
    else if (isset($form['items'][$element_id]) && !empty($form['items'][$element_id])) {
      $context->{$element_id}[] = $form['items'][$element_id];
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
 * Submit handler for main context_ui form.
 */
function context_ui_form_submit($form, &$form_state) {

  switch ($form_state['clicked_button']['#id']) {
    // Send user to delete confirmation page
    case 'edit-delete':
      $form_state['redirect'] = 'admin/build/context/delete/'. $form_state['values']['cid'];
      return;
    // Process form values and save and/or update the context in the db
    case 'edit-submit':
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

  foreach ($blocks as $r => $sort_region) {
    if ($r !== 0) {
      uasort($sort_region, "_context_ui_block_compare");
      $blocks[$r] = $sort_region;
    }
  }

  return $region ? $blocks[$region] : $blocks;
}
