// $Id$

if (typeof(Drupal) == "undefined" || !Drupal.context_prefix_admin) {
  Drupal.context_prefix_admin = {};
}

Drupal.context_prefix_admin.attach = function() {
  $('select[id^="edit-context-prefix-"]').change(function(i){
    Drupal.context_prefix_admin.alter(this);
  }).each(function(i){
    Drupal.context_prefix_admin.alter(this);
  });
}

Drupal.context_prefix_admin.alter = function(elem){
  if (elem.value === '3') {
    $(elem).parents('tr').find('input[id^="edit-context-prefix-"]').show();
  }
  else {
    $(elem).parents('tr').find('input[id^="edit-context-prefix-"]').hide();
  }
}

if (Drupal.jsEnabled) {
  $(document).ready(function() {
    if ($('form#context-prefix-settings-form').size() > 0) {
      Drupal.context_prefix_admin.attach();
    }
  });
};