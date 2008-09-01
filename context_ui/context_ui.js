// $Id$

if (typeof(Drupal) == "undefined" || !Drupal.context_ui) {
  Drupal.context_ui = {};
}

Drupal.context_ui.attach = function() {
  var item_tools = "<div class='tools'><span class='up'>Up</span><span class='down'>Down</span><span class='remove'>X</span></div>";

  // multiselect handler
  $("input#edit-block-selector-add").click(function() {
    var region = $("select#edit-block-selector-regions").val().replace('_', '-');
    var selected = $("select#edit-block-selector-blocks option:selected");
    if (selected.size() > 0) {
      $("div.context-ui-block-regions ul." + region + " li.dummy").remove();
      selected.each(function() {
        if (!$(this).attr('disabled')) {
          // create new block li
          var block = document.createElement('li');
          var value = $(this).attr('value');
          var text = item_tools + $(this).text();
          $(block).attr('title', value).html(text);

          // attach tool handlers
          Drupal.context_ui.attachtools(block);

          // remove option
          $(this).remove();

          // add block item to region
          $("div.context-ui-block-regions ul."+ region).append(block);

          Drupal.context_ui.regionblocks(region);
        }
      });
    }
  });

  // attach tool handler to existing context_ui blocks
  $("div.context-ui-block-regions ul li").each(function() {
    Drupal.context_ui.attachtools(this);
  });
}

Drupal.context_ui.attachtools = function(block) {
  if ($("div.tools", block).size() > 0) {
    // remove block
    $("div.tools span.remove", block).click(function() {
      var item = $(this).parents("li");
      $("div.tools", item).remove();

      // create new block select option
      var option = document.createElement('option');
      var value = $(item).attr('title');
      var text = item.text();
      $(option).attr('value', value).text(text);

      // retrieve region info before item is deleted
      var region = $(item).parents("ul").attr("class");

      // remove block item
      item.remove();

      // add block option
      $("select#edit-block-selector-blocks").append(option);

      Drupal.context_ui.regionblocks(region);
    });
    // move block up
    $("div.tools span.up", block).click(function() {
      var prev = $(this).parents("li").prev();
      if (prev) {
        var item = $(this).parents("li");
        var region = $(this).parents("ul").attr("class");
        prev.before(item);
        Drupal.context_ui.regionblocks(region);
      }
    });
    // move block down
    $("div.tools span.down", block).click(function() {
      var next = $(this).parents("li").next();
      if (next) {
        var item = $(this).parents("li");
        var region = $(this).parents("ul").attr("class");
        next.after(item);
        Drupal.context_ui.regionblocks(region);
      }
    });
  }
}

Drupal.context_ui.regionblocks = function (region) {
  var serialized = '';
  if ($("div.context-ui-block-regions ul."+ region +" li").size() > 0) {
    $("div.context-ui-block-regions ul."+ region +" li").each(function() {
      if (serialized == '') {
        serialized = $(this).attr('title');
      }
      else {
        serialized = serialized +","+ $(this).attr('title');
      }
    });
    $("input#edit-block-regions-"+ region).val(serialized);
  }
  else if ($("input#edit-block-regions-"+ region).size() > 0) {
    $("input#edit-block-regions-"+ region).val('');
  }
}

if (Drupal.jsEnabled) {
  $(document).ready(function() {
    if ($('form#context-ui-form').size() > 0) {
      Drupal.context_ui.attach();
    }
  });
};
