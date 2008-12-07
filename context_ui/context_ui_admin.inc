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
 * Recursive function that intelligently populates default values in a
 * form from a provided array of data.
 * 
 * @param $form
 *   A form API element to populate with default values.
 * @param $data
 *   A keyed array of data that matches the tree structure of the
 *   form API branch it should populate.
 * 
 * @return
 *   A form API element populated with default values.
 */
function context_ui_populate_defaults($form, $data) {
  if (!empty($form['#tree'])) {
    foreach (element_children($form) as $id) {
      if (isset($data[$id])) {
        $form[$id] = context_ui_populate_defaults($form[$id], $data[$id]);
      }
    }
  }
  else {
    if (is_array($data) && $form['#type'] == 'checkboxes') {
      $form['#default_value'] = array();
      foreach ($data as $value) {
        $form['#default_value'][$value] = $value;
      }
    }
    else if (is_array($data) && $form['#type'] == 'select' && $form['#multiple'] == true) {
      $form['#default_value'] = array();
      foreach ($data as $value) {
        $form['#default_value'][] = $value;
      }
    }
    else {
      $form['#default_value'] = is_array($data) ? current($data) : $data;
    }
  }
  return $form;
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
function context_ui_form(&$form_state, $op, $context = NULL) {
  switch ($op) {
    case 'add':
      if (is_object($context)) {
        $context->system = true;
        if ($exists = context_ui_load($context)) {
          drupal_set_message(t('A module is already providing a context with this space/key/value identifier. Your context definition will override its settings.'));
        }
        $context->system = false;
      }
      break;
    case 'edit':
      if (!$context->system) {
        drupal_set_title(t('Edit context: %title', array('%title' => $context->value)));
      }
      else {
        drupal_set_title(t('View %title', array('%title' => $context->value)));
      }
      break;
    case 'clone':
      drupal_set_title(t('Clone context: %title', array('%title' => $context->value)));
      $context->system = 0;
      $context->cid = null;
      $cid = null;
      break;
  }

  // Initialize context object if it doesn't already exist
  $context = !$context ? new StdClass() : $context;

  // Core context definition
  $form = array(
    '#base' => 'context_ui_form',
    '#theme' => 'context_ui_form',
  );

  $form['cid'] = array(
    '#type' => 'value',
    '#value' => isset($context->cid) ? $context->cid : NULL,
  );

  $form['system'] = array(
    '#type' => 'value',
    '#value' => isset($context->system) ? $context->system : 0,
  );

  foreach (array('value', 'attribute', 'namespace') as $field) {
    $form[$field] = array(
      '#type' => 'textfield',
      '#required' => true,
      '#maxlength' => 64,
      '#size' => 20,
      '#disabled' => !empty($context->system) ? true : false,
      '#default_value' => isset($context->{$field}) ? $context->{$field} : '',
    );
  }
  $form['section'] = $form['value'];

  // The 'section' value is a field to make context definition creation
  // less confusing for basic users.
  $form['section']['#required'] = FALSE;
  $form['section']['#title'] = t('Section');
  $form['section']['#description'] = t('Your site\'s section. !advanced', array('!advanced' => l(t('Toggle advanced settings'), $_GET['q'], array('fragment' => 'advanced', 'attributes' => array('class' => 'advanced-toggle')))));

  $form['value']['#title'] = t('Value');
  $form['value']['#description'] = t('A system name for this context. May only contain lowercase letters, underscores, and numbers. Example: <b>science_blog</b>');
  
  $form['attribute']['#title'] = t('Attribute');
  $form['attribute']['#default_value'] = empty($form['attribute']['#default_value']) ? 'section' : $form['attribute']['#default_value'];
  $form['attribute']['#description'] = t('The type of context information provided in this namespace. Example: <b>section</b>');

  $form['namespace']['#title'] = t('Namespace');
  $form['namespace']['#default_value'] = empty($form['namespace']['#default_value']) ? 'context_ui' : $form['namespace']['#default_value'];
  $form['namespace']['#description'] = t('The namespace for this context definition. Example: <b>context_ui</b>');

  $form['items'] = array('#tree' => true);

  // We need to initialize theme in order to deal with blocks
  // and also let themes integrate against context_ui
  init_theme();
  $theme_key = variable_get('theme_default', 'garland');

  // Generate settings for context item associations
  foreach ((context_ui_setters(TRUE) + context_ui_getters(TRUE)) as $id => $info) {
    $form['items'][$id] = $info;

    // Disable element
    if (isset($context->system) && $context->system) {
      $form['items'][$id]['#disabled'] = true;
    }

    // Default values
    if (isset($context->{$id})) {
      $form['items'][$id] = context_ui_populate_defaults($form['items'][$id], $context->{$id});
    }
  }

  // Control block visibility
  $block_options =
  $block_defaults = array();
  $blocks = _context_ui_get_blocks();
  $regions = system_region_list($theme_key);
  // $blocks in [0] have not been assigned a region
  foreach ($blocks[0] as $block) {
    $block_options[$block->module][$block->bid] = $block->label ." ($block->bid)";
  }
  ksort($block_options);

  $form['block'] = array(
    '#tree' => true,
  );

  $form['block']['help'] = array(
    '#type' => 'markup',
    '#value' => t('Control block visibility using context. Selected blocks will be shown when this context is set provided that custom block visibility settings and/or throttling do not hide them. Grayed out blocks are those provided by Drupal\'s standard block settings. These settings apply to the current theme and any enabled themes with regions in common.'),
  );

  $form['block']['selector'] = array(
    '#type' => 'item',
    '#tree' => true,
    '#prefix' => '<div class="context-ui-block-selector">',
    '#suffix' => '</div>',
  );
  foreach ($block_options as $module => $module_blocks) {
    $form['block']['selector'][$module] = array(
      '#type' => 'checkboxes',
      '#title' => $module,
      '#options' => $module_blocks,
      '#disabled' => isset($context->system) && $context->system ? true : false,
    );
  }

  $form['block']['blocks'] = array(
    '#tree' => true,
    '#theme' => 'context_ui_block_ui',
  );
  foreach (array_keys($regions) as $region) {
    $defaults = array();
    $midpoint = false;
    $form['block']['blocks'][$region] = array(
      '#type' => 'item',
      '#title' => $region,
      '#tree' => TRUE,
    );

    $system = _context_ui_get_blocks($region);
    if ($system) {
      $system_blocks = array();
      foreach ($system as $block) {
        $system_blocks[] = $block->label ." ($block->bid)";
      }
      $system_blocks = "<span class='system-blocks'>". implode(", ", $system_blocks) ."</span";
      $form['block']['blocks'][$region]['system'] = array(
        '#type' => 'markup',
        '#tree' => true,
        '#value' => $system_blocks,
        '#weight' => 0,
        'weight' => array(
          '#type' => 'weight',
          '#delta' => 20,
          '#default_value' => 0,
        ),
      );
    }

    $i = 0;
    foreach (_context_ui_get_blocks($region, $context) as $block) {
      if ($block->type == 'context_ui') {
        $form['block']['blocks'][$region][$block->bid] = array(
          '#type' => 'markup',
          '#tree' => true,
          '#value' => $block->label,
          '#weight' => $block->weight,
          'weight' => array(
            '#type' => 'weight',
            '#delta' => 20,
            '#default_value' => $block->weight,
          ),
        );
      }
      $i++;
    }

    uasort($form['block']['blocks'][$region], 'element_sort');

    $defaults = implode(',', element_children($form['block']['blocks'][$region]));
    $form['block']['regions'][$region] = array(
      '#type' => 'hidden',
      '#default_value' => $defaults,
    );
  }

  if (isset($context->system) && $context->system) {
    $form['back'] = array(
      '#type' => 'item',
      '#value' => l(t('Back'), 'admin/build/context'),
    );
  }
  else {
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

  return $form;
}

/**
 * Generates an abbreviated list of items for display in the
 * setter/getter UI.
 */
function context_ui_item_display($type, $element) {
  // We're dealing with an item with options --
  // try to grab the display-friendly value
  $items = array();
  $title = l($element['#title'], $_GET['q'], array('fragment' => $type));
  if (isset($element['#options'])) {
    if (isset($element['#default_value'])) {
      if (is_array($element['#default_value'])) {
        foreach ($element['#default_value'] as $k) {
          $items[] = $element['#options'][$k];
        }
      }
      else if (is_string($element['#default_value']) && $k = $element['#default_value']) {
        $items[] = $element['#options'][$k];
      }
    }
    if (empty($items)) {
      $items[] = array('data' => '', 'class' => 'empty');
    }
  }
  else if (isset($element['#type']) && in_array($element['#type'], array('textfield', 'textarea'))) {
    $items[] = !empty($element['#default_value']) ? $element['#default_value'] : array('data' => '', 'class' => 'empty');
  }
  return theme('item_list', $items, $title, 'ul', array('id' => 'display-'. $type));
}

/**
 * Theme function for context_ui_form()
 */
function theme_context_ui_form($form) {
  drupal_add_css(drupal_get_path("module", "context_ui") ."/context_ui.css");
  drupal_add_js(drupal_get_path("module", "context_ui") ."/context_ui.js");

  $output = '';


  // Render space / key / value trio in a 3-column table
  $rows = array(
    'section' => array('data' => array(), 'class' => 'section'),
    'trio' => array('data' => array(), 'class' => 'trio'),
  );
  if ($form['namespace']['#default_value'] == 'context_ui' && $form['attribute']['#default_value'] == 'section') {
    $rows['trio']['class'] .= ' hidden';
  }
  else {
    // Not sure why this first flag doesn't work -- hey are we too
    // late in the rendering stack?
    $form['section']['#disabled'] = true;
    $form['section']['#attributes'] = array('disabled' => true);
  }
  $rows['section']['data'][] = array('data' => drupal_render($form['section']), 'colspan' => 3);
  $rows['trio']['data'][] = array('data' => drupal_render($form['namespace']), 'class' => 'namespace');
  $rows['trio']['data'][] =  array('data' => drupal_render($form['attribute']), 'class' => 'attribute');
  $rows['trio']['data'][] = array('data' => drupal_render($form['value']), 'class' => 'value');
  $output .= theme('table', array(), $rows, array('id' => 'context-ui-trio'));


  // Render setters / getters as a two column split
  $widgets = '';
  $display = "<h1>". t('Set context') ."</h1>";
  foreach (array_keys(context_ui_setters()) as $id) {
    $widgets .= "<div id='widget-$id' class='widget'>". drupal_render($form['items'][$id]) ."</div>";
    $display .= context_ui_item_display($id, $form['items'][$id]);
  }
  $display .= "<h1>". t('Respond to context') ."</h1>";
  foreach (array_keys(context_ui_getters()) as $id) {
    $widgets .= "<div id='widget-$id' class='widget'>". drupal_render($form['items'][$id]) ."</div>";
    $display .= context_ui_item_display($id, $form['items'][$id]);
  }
  $rows = array(
    array(
      array('data' => $display, 'class' => 'display'),
      array('data' => $widgets, 'class' => 'widget'),
    ),
  );
  $output .= theme('table', array(), $rows, array('id' => 'context-ui-items'));


  // Render block visibility
  $rows = array(
    array(
      array('data' => drupal_render($form['block']['blocks']), 'class' => 'display'),
      array('data' => drupal_render($form['block']['selector']) . drupal_render($form['block']['help']), 'class' => 'widget'),
    ),
  );
  $output .= theme('table', array(), $rows, array('id' => 'context-ui-blocks'));

  $output .= drupal_render($form);
  return $output;
}

/**
 * Provide a form to confirm one of the provided actions.
 */
function context_ui_confirm(&$form_state, $op = 'delete', $context) {
  $form = array();
  $form['cid'] = array('#type' => 'value', '#value' => $context->cid);
  $form['action'] = array('#type' => 'value', '#value' => $op);
  switch ($op) {
    case 'delete':
      $action = t('delete');
      $message = t('This action will remove this context permanently from your site.');
      break;
    case 'disable':
      $action = t('disable');
      $message = '';
      break;
    case 'enable':
      $action = t('enable');
      $message = '';
      break;
  }
  $form = confirm_form($form,
    t('Are you sure you want to !action the context %title?', array('%title' => $context->value, '!action' => $action)),
    'admin/build/context',
    $message,
    drupal_ucfirst($action), t('Cancel')
  );
  return $form;
}

/**
 * Submit handler for the context_ui_confirm form.
 */
function context_ui_confirm_submit($form, &$form_state) {
  switch ($form_state['values']['action']) {
    case 'delete':
      context_ui_delete($form_state['values']['cid']);
      context_ui_rebuild();
      break;
    case 'disable':
      $context = context_ui_load($form_state['values']['cid']);
      if ($context) {
        $context->status = CONTEXT_DISABLED;
        context_ui_save($context);
        context_ui_rebuild();
      }
      break;
    case 'enable':
      $context = context_ui_load($form_state['values']['cid']);
      if ($context) {
        $context->status = CONTEXT_ENABLED;
        context_ui_save($context);
        context_ui_rebuild();
      }
      break;
  }
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
    if ($exists = context_ui_load($context)) {
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
function context_ui_export(&$form_state, $context) {
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

  // build the form
  $form = array();
  $form['help'] = array(
    '#type' => 'item',
    '#value' => $help,
  );
  $form['export'] = array(
    '#type' => 'textarea',
    '#rows' => 24,
    '#value' => $context,
    '#theme' => 'context_ui_export_form',
  );
  return $form;
}

/**
 * Themes a context value into an export friendly var_export().
 */
function theme_context_ui_export_form($form) {
  $form['#value'] =
  $form['#default_value'] = '$items[] = '. var_export($form['#value'], true) .';';
  return theme('textarea', $form);
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
  cache_clear_all('context_setters', 'cache');
  cache_clear_all('context_getters', 'cache');
  $default_contexts = context_ui_defaults();

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
      $context = context_ui_load($c, TRUE);

      // If context is already present in db, merge salient fields from
      // db version and use the rest from code.
      if ($context) {        

        // If status has been stored as overridden but user context no longer
        // exists, we need to re-enable this system context.
        if ($context->status == CONTEXT_OVERRIDDEN && !isset($ui[$c->namespace][$c->attribute][$c->value])) {
          $c->status = CONTEXT_ENABLED;
        }
        // Set to overridden any contexts that were enabled
        else if ($context->status == CONTEXT_ENABLED && isset($ui[$c->namespace][$c->attribute][$c->value])) {
          $c->status = CONTEXT_OVERRIDDEN;
        }
        // Otherwise, just use the stored status.
        else {
          $c->status = $context->status;
        }

        $c->cid = $context->cid;
        context_ui_save($c);
        unset($system[$c->cid]); // remove this context from unused list
      }
      else {
        // Check that the providing module has not explicitly set the status.
        if (!isset($c->status)) {
          $c->status = isset($ui[$c->namespace][$c->attribute][$c->value]) ? CONTEXT_OVERRIDDEN : CONTEXT_ENABLED;
        }
        context_ui_save($c);
      }
    }
  }
  
  // Remove any unused contexts
  foreach ($system as $c => $dummy) {
    context_ui_delete($c);
  }
}

/**
 * Generates the main context_ui admin page with a tiered context listing.
 */
function theme_context_ui_admin($context_tree) {
  foreach ($context_tree as $namespace => $attributes) {
    $rows[] = array(
      array('data' => "<span class='context-namespace'>". $namespace ."</span>", 'class' => 'context'),
      null,
      null,
    );
    foreach ($attributes as $attribute => $contexts) {
      if (is_array($contexts)) {
        $rows[] = array(
          "<span class='context-attribute'>". $attribute ."</span>",
          null,
          null,
        );
        foreach ($contexts as $value => $context) {
          $links = array();          
          $class = $enable = '';
          if ($context->system) {
            $links[] = l(t('View'), "admin/build/context/$context->cid");
            $links[] = l(t('Override'), "admin/build/context/$context->cid/clone");
          }
          else {
            $links[] = l(t('Edit'), "admin/build/context/$context->cid");
            $links[] = l(t('Export'), "admin/build/context/$context->cid/export");
            $links[] = l(t('Clone'), "admin/build/context/$context->cid/clone");
            $links[] = l(t('Delete'), "admin/build/context/$context->cid/delete");
          }
          switch ($context->status) {
            case CONTEXT_DISABLED:
              $class = 'disabled';
              $value = "$value (". t('Disabled') .")";
              $enable = l(t('Enable'), "admin/build/context/$context->cid/enable");
              break;
            case CONTEXT_OVERRIDDEN:
              $class = 'disabled';
              $value = "$value (". t('Overridden') .")";
              break;
            default:
              $enable = l(t('Disable'), "admin/build/context/$context->cid/disable");
              break;
          }
          $rows[] = array(
            "<span class='context-value $class'>". $value ."</span>",
            $enable,
            implode(' | ', $links),
          );
        }
      }
    }
  }
  return theme('table', array(t('Context'),t('Status'),t('Actions')), $rows, array('class' => 'context-ui-overview'));
}

/**
 * Generates the AJAX enabled block administration portion of the context_ui admin form.
 */
function theme_context_ui_block_ui($form) {
  // Add draggable weights
  drupal_add_js('misc/tableheader.js');

  $output = '';

  foreach (element_children($form) as $region) {
    $table_id = 'context-ui-region-'. str_replace('_', '-', $region);
    drupal_add_tabledrag($table_id, 'order', 'sibling', 'block-weight', NULL, NULL, FALSE);
    $rows = array();
    foreach (element_children($form[$region]) as $id) {
      $form[$region][$id]['weight']['#attributes'] = array('class' => 'block-weight');
      $label = $form[$region][$id]['#value'] ." ($id)";
      if ($id == 'system') {
        $remove = '';
      }
      else {
        $remove = l(t('X'), $_GET['q'], array('fragment' => 'remove', 'attributes' => array('class' => 'remove')));
      }
      $rows[] = array(
        'data' => array($label . drupal_render($form[$region][$id]['weight']), $remove),
        'class' => 'draggable',
        'id' => $id,
      );
    }
    $output .= "<h1>". l(t('+ Add'), $_GET['q'], array('fragment' => $region, 'attributes' => array('class' => 'add-block'))) . $region ."</h1>";
    $output .= theme('table', array(), $rows, array('id' => $table_id));
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
      if ($exists = context_ui_load($context)) {
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

  // Retrieve values for setters
  foreach (array_keys(context_ui_setters()) as $id) {
    if (is_array($form['items'][$id])) {
      foreach ($form['items'][$id] as $option => $value) {
        if ($value) {
          $context->{$id}[] = $option;
        }
      }
    }
    else if (isset($form['items'][$id]) && !empty($form['items'][$id])) {
      $context->{$id}[] = $form['items'][$id];
    }
  }

  // Retrieve values for getters -- preserve key => val associations
  foreach (array_keys(context_ui_getters()) as $id) {
    if (isset($form['items'][$id]) && !empty($form['items'][$id])) {
      $context->{$id} = $form['items'][$id];
    }
  }

  // Blocks must be done by region
  $context->block = array();
  $theme_key = variable_get('theme_default', 'garland');
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
      $form_state['redirect'] = 'admin/build/context/'. $form_state['values']['cid'] .'/delete';
      return;
    // Process form values and save and/or update the context in the db
    case 'edit-submit':
      $context = context_ui_form_process($form_state['values']);
      $result = context_ui_save($context);
      if ($result) {
        drupal_set_message(t('The context %title was saved successfully.', array('%title' =>  $context->value)));
      }
      else {
        drupal_set_message(t('An error occurred while attempting to save your context information.'), 'error');
      }
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
  $theme_key = variable_get('theme_default', 'garland');
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
  if (is_object($context) && !empty($context->block)) {
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
      uasort($sort_region, create_function('$a, $b', 'return ($a->weight - $b->weight);'));
      $blocks[$r] = $sort_region;
    }
  }

  return $region ? $blocks[$region] : $blocks;
}
