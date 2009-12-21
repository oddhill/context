// $Id$

Drupal.behaviors.contextReactionBlock = function(context) {
  // Context uses Drupal behaviors as an informal hook/events system.
  if (context.caller == 'contextEditor' && context.editor && context.event) {
    switch (context.event) {
      case 'init':
        Drupal.contextBlockEditor = new DrupalContextBlockEditor(context.editor);
        break;
      case 'editStart':
        Drupal.contextBlockEditor.editStart(context.editor, context.context);
        break;
      case 'editFinish':
        Drupal.contextBlockEditor.editFinish();
        break;
    }
    return;
  }

  //
  // Editor ===========================================================
  //
  // Attach handlers to editable blocks.
  // This lives outside the block editor class as it may needs to be
  // called each time Drupal.attachBehaviors() is called.
  $('div.context-block:not(.processed)').each(function() {
    $('a.remove', $(this)).click(function() {
      $(this).parents('div.context-block').remove();
      Drupal.contextBlockEditor.updateBlocks();
      return false;
    });
  });

  // 
  // Admin Form =======================================================
  //
  // ContextBlockForm: Init.
  $('#context-blockform:not(.processed)').each(function() {
    $(this).addClass('processed');
    Drupal.contextBlockForm = new DrupalContextBlockForm($(this));
    Drupal.contextBlockForm.setState();
  });

  // ContextBlockForm: Attach block removal handlers.
  // Lives in behaviors as it may be required for attachment to new DOM elements.
  $('#context-blockform a.remove:not(.processed)').each(function() {
    $(this).addClass('processed');
    $(this).click(function() {
      $(this).parents('tr').eq(0).remove();
      Drupal.contextBlockForm.setState();
      return false;
    });
  });
};

/**
 * Context block form. Default form for editing context block reactions.
 */
function DrupalContextBlockForm(blockForm) {
  this.state = {};

  this.setState = function() {
    $('table.context-blockform-region', blockForm).each(function() {
      var region = $(this).attr('id').split('context-blockform-region-')[1];
      var blocks = [];
      $('tr', $(this)).each(function() {
        var bid = $(this).attr('id');
        blocks.push(bid);
      });
      Drupal.contextBlockForm.state[region] = blocks;
    });

    // Serialize here and set form element value.
    $('form input.context-blockform-state').val(JSON.stringify(this.state));

    // Hide enabled blocks from selector that are used
    $('table.context-blockform-region tr').each(function() {
      var bid = $(this).attr('id');
      $('div.context-blockform-selector input[value='+bid+']').parents('div.form-item').eq(0).hide();
    });
    // Show blocks in selector that are unused
    $('div.context-blockform-selector input').each(function() {
      var bid = $(this).val();
      if ($('table.context-blockform-region tr#'+bid).size() === 0) {
        $(this).parents('div.form-item').eq(0).show();
      }
    });
  };

  // Tabledrag
  // Add additional handlers to update our blocks.
  for (var base in Drupal.settings.tableDrag) {
    var table = $('#' + base + ':not(.processed)', blockForm);
    if (table && table.is('.context-blockform-region')) {
      table.addClass('processed');
      table.bind('mouseup', function(event) {
        Drupal.contextBlockForm.setState();
        return;
      });
    }
  }

  // Add blocks to a region
  $('td.blocks a', blockForm).each(function() {
    $(this).click(function() {
      var region = $(this).attr('href').split('#')[1];
      var selected = $("div.context-blockform-selector input:checked");
      if (selected.size() > 0) {
        selected.each(function() {
          // create new block markup
          var block = document.createElement('tr');
          var text = $(this).parents('div.form-item').eq(0).hide().children('label').text();
          $(block).attr('id', $(this).attr('value')).addClass('draggable');
          $(block).html("<td>"+ text + "<input class='block-weight' /></td><td><a href='' class='remove'>X</a></td>");

          // add block item to region
          var base = "context-blockform-region-"+ region;
          Drupal.tableDrag[base].makeDraggable(block);
          $('table#'+base).append(block);
          Drupal.attachBehaviors($('table#'+base));

          Drupal.contextBlockForm.setState();
          $(this).removeAttr('checked');
        });
      }
      return false;
    });
  });
}

/**
 * Context block editor. AHAH editor for live block reaction editing.
 */
function DrupalContextBlockEditor(editor) {
  this.state = {};

  /**
   * Update UI to match the current block states.
   */
  this.updateBlocks = function() {
    var browser = $('div.context-block-browser');

    // For all enabled blocks, hide corresponding addables.
    $('div.block, div.admin-block').each(function() {
      var bid = $(this).attr('id').split('block-')[1]; // Ugh.
      $('#context-block-addable-'+bid, browser).hide();
    });
    // For all hidden addables with no corresponding blocks, show.
    $('.context-block-addable', browser).each(function() {
      var bid = $(this).attr('id').split('context-block-addable-')[1];
      if ($('#block-'+bid).size() === 0) {
        $(this).show();
      }
    });
    // Mark empty regions.
    $('.context-block-region').each(function() {
      // Clean up after jQuery UI. Sometimes addables get left in the regions -- not good.
      $('.context-block-addable', this).remove();

      if ($('div.context-block', this).size() > 0) {
        $(this).removeClass('context-block-region-empty');
      }
      else {
        $(this).addClass('context-block-region-empty');
      }
    });
  };

  /**
   * Add a block to a region through an AHAH load of the block contents.
   */
  this.addBlock = function(event, ui, editor, context) {
    // Remove empty regionism early.
    editor.removeClass('context-block-region-empty');

    if (ui.item.is('.context-block-addable')) {
      var bid = ui.item.attr('id').split('context-block-addable-')[1];
      var params = {
        'path': Drupal.settings.contextBlockEditor.path,
        'bid': bid,
        'context': context
      };
      $.getJSON(Drupal.settings.contextBlockEditor.ajax, params, function(data) {
        if (data.status) {
          var newBlock = $(data.block);
          newBlock.addClass('draggable');
          var newBlock = ui.item.replaceWith(newBlock);

          $.each(data.css, function(k, v){
            var cssfile = Drupal.settings.basePath + v;
            if ($('head link[href $='+cssfile+']').length == 0 ) {
              $('head').append('<link type="text/css" rel="stylesheet" media="all" href="' + cssfile + " />'");
            }
          });

          Drupal.contextBlockEditor.updateBlocks();
          Drupal.attachBehaviors();
        }
        else {
          ui.item.remove();
        }
      });
    }
    else if (ui.item.is('.context-block')) {
      Drupal.contextBlockEditor.updateBlocks();
    }
  };

  /**
   * Update form hidden field with JSON representation of current block visibility states.
   */
  this.setState = function() {
    $('div.context-block-region').each(function() {
      var region = $(this).attr('id').split('context-block-region-')[1];
      var blocks = [];
      $('div.context-block', $(this)).each(function() {
        var bid = $(this).attr('id').split('context-block-')[1];
        var context = $(this).attr('class').split('edit-')[1];
        var context = context ? context : 0;
        var block = {'bid': bid, 'context': context};
        blocks.push(block);
      });
      Drupal.contextBlockEditor.state[region] = blocks;
    });

    // Serialize here and set form element value.
    $('form.context-editor input.context-block-editor-state').val(JSON.stringify(this.state));
  };

  /**
   * Start editing. Attach handlers, begin draggable/sortables.
   */
  this.editStart = function(editor, context) {
    $('div.context-block-region > div.edit-'+context).addClass('draggable');

    // First pass, enable sortables on all regions.
    var params = {
      dropOnEmpty: true,
      placeholder: 'draggable-placeholder',
      forcePlaceholderSize: true,
      stop: function(event, ui) { Drupal.contextBlockEditor.addBlock(event, ui, editor, context); }
    }
    $('div.context-block-region').sortable(params);

    // Second pass, hook up all regions via connectWith to each other.
    $('div.context-block-region').each(function() {
      $(this).sortable('option', 'connectWith', ['div.context-block-region']);
    });
  };

  /**
   * Finish editing. Remove handlers.
   */
  this.editFinish = function() {
    $('div.context-block-region > div.draggable').removeClass('draggable');
    $('div.context-block-region').sortable('destroy');
    this.setState();
  };

  // Category selector handler.
  // Also set to "Choose a category" option as browsers can retain
  // form values from previous page load.
  $('select.context-block-browser-categories', editor).val(0).change(function() {
    var category = $(this).val();
    $('div.category').hide();
    $('div.category-'+category).show();
  });

  // Add draggable handler for addables.
  var options = {
    appendTo: 'body',
    helper: 'clone',
    zIndex: '2700',
    connectToSortable: ['div.context-block-region'],
    start: function(event, ui) { $(document.body).addClass('context-block-adding'); },
    stop: function(event, ui) { $(document.body).removeClass('context-block-adding'); }
  };
  $('div.context-block-addable', editor).draggable(options);

  // Set the block states.
  this.updateBlocks();
};
