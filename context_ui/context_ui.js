// $Id$

/**
 * Context plugin form.
 */
function DrupalContextPlugins(form) {
  this.form = form;

  // Sync the form selector and state field with the list of plugins currently enabled.
  this.setState = function() {
    var state = [];
    $('.context-plugin-list > li', this.form).each(function() {
      var plugin = $(this).attr('class').split('context-plugin-')[1].split(' ')[0];
      if ($(this).is('.disabled')) {
        $('.context-plugin-selector select option[value='+plugin+']', this.form).show();
      }
      else {
        state.push(plugin);
        $('.context-plugin-selector select option[value='+plugin+']', this.form).hide();
      }
    });
    // Set the hidden plugin list state.
    $('.context-plugin-selector input.context-plugins-state', this.form).val(state.join(','));

    // Reset the selector.
    $('.context-plugin-selector select', this.form).val(0);
    return this;
  };

  // Add a plugin to the list.
  this.addPlugin = function(plugin) {
    $('.context-plugin-list > li.context-plugin-'+plugin, this.form).removeClass('disabled');
    this.showForm(plugin).setState();
    return this;
  };

  // Remove a plugin from the list.
  this.removePlugin = function(plugin) {
    $('.context-plugin-list > li.context-plugin-'+plugin, this.form).addClass('disabled');
    this.hideForm(plugin).setState();
    return this;
  };

  // Show a plugin form.
  this.showForm = function(plugin) {
    $('.context-plugin-forms > .context-plugin-form.active-form', this.form).removeClass('active-form');
    $('.context-plugin-forms > .context-plugin-form-'+plugin, this.form).addClass('active-form');
    $('.context-plugin-list > li > a').removeClass('active-form');
    $('.context-plugin-list > li.context-plugin-'+plugin+' > a').addClass('active-form');
    return this;
  };

  // Show a plugin form.
  this.hideForm = function(plugin) {
    $('.context-plugin-forms > .context-plugin-form-'+plugin, this.form).removeClass('active-form');
    $('.context-plugin-list > li.context-plugin-'+plugin+' > a').removeClass('active-form');
    return this;
  };

  // Select handler.
  $('.context-plugin-selector select', this.form).change(function() {
    var plugins = $(this).parents('div.context-plugins').data('contextPlugins');
    if (plugins) {
      var plugin = $(this).val();
      plugins.addPlugin(plugin);
    }
  });

  // Show form handler.
  $('.context-plugin-list > li > a', this.form).click(function() {
    var plugins = $(this).parents('div.context-plugins').data('contextPlugins');
    if (plugins) {
      var plugin = $(this).attr('href').split('#context-plugin-form-')[1];
      plugins.showForm(plugin);
    }
    return false;
  });

  // Remove handler.
  $('.context-plugin-list span.remove', this.form).click(function() {
    var plugins = $(this).parents('div.context-plugins').data('contextPlugins');
    if (plugins) {
      var plugin = $(this).parent().attr('href').split('#context-plugin-form-')[1];
      plugins.removePlugin(plugin);
    }
    return false;
  });

  // Set the plugin states.
  this.setState();
}

/**
 * Context editor. AHAH editor for live context editing.
 */
function DrupalContextEditor(editor) {
  this.context = '';
  this.editing = false;
  this.state = {};

  this.invoke = function(type) {
    var event = {
      'caller': 'contextEditor',
      'event': type,
      'editor': editor,
      'context': this.context
    };
    Drupal.attachBehaviors(event);
  };

  this.editStart = function(context) {
    if (!this.editing) {
      this.editing = true;
      this.context = context;
      $('#context-editable-'+context, editor).show();
      $(document.body).addClass('context-editing');
      this.invoke('editStart');
    }
  };

  this.editFinish = function() {
    if (this.editing) {
      this.editing = false;
      this.context = '';
      $(document.body).removeClass('context-editing');
      $('div.contexts div.context-editable', editor).hide();
      $('li.context-editable').removeClass('context-editing');

      // Indicate that edits have been made.
      $('form.context-editor').addClass('edited');
      this.invoke('editFinish');
    }
  };

  // Attach handlers to editable contexts.
  $('li.context-editable a.edit', editor).click(function() {
    var trigger = $(this).parents('li.context-editable').addClass('context-editing');
    var context = trigger.attr('id').split('context-editable-trigger-')[1];
    Drupal.contextEditor.editStart(context);
    return false;
  });
  $('li.context-editable a.done', editor).click(function() {
    Drupal.contextEditor.editFinish();
    return false;
  });
  this.invoke('init');
}

Drupal.behaviors.context_ui = function(context) {
  // Initialize context plugin form.
  $('form div.context-plugins:not(.context-ui-processed)').each(function() {
    $(this).addClass('context-ui-processed');
    $(this).data('contextPlugins', new DrupalContextPlugins($(this)));
  });

  // Initialize context editor.
  $('form div.context-editor:not(.context-ui-processed)').each(function() {
    $(this).addClass('context-ui-processed');
    Drupal.contextEditor = new DrupalContextEditor($(this));
  });
};
