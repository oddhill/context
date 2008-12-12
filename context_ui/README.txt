$Id$

context_ui for Drupal 6.x

Installation
------------
context_ui can be installed like any other Drupal module -- place it in
the modules directory for your site and enable it (and its requirement,
context) on the admin/build/modules page.

You will probably want to install context_ui_contrib as well, which
adds context_ui integration for several contrib modules including
Views.

Basic usage
-----------
context_ui allows you to manage contexts for different portions of your
site. You can think of each context as represeting a "section" of your
site. For each context, you can choose the conditions that trigger this
context to be active and choose different aspects of Drupal that should
respond to this active context.

Example
-------
You want to create a "pressroom" section of your site. You have a press
room view that displays press release nodes, but you also want to tie
a book with media resources tightly to this section. You would also
like a contact block you've made to appear whenever a user is in the
pressroom section.

1. Add a new context on admin/build/context
2. Set the section identifier / value to "pressroom"
3. Under the "set context" dialogue, associate the pressroom nodetype,
   the pressroom view, and the media kit book with the context.
4. Choose the pressroom menu item to be set active under the "respond
   to context" items.
5. Add the contact block to a region under the block visibility
   settings.
6. Save the context.

Hooks
-----
hook_context_ui_setters()
  Provides an array of FormAPI definitions. Allows you to provide
  additional conditions as context setters in the context UI form.
  See context_ui_context_ui_setters() as an example.

hook_context_ui_getters()
  Provides an array of FormAPI definitions. Allows you to provide
  additional context getters that respond to a set context in the
  context UI form. See context_ui_context_ui_getters() as an
  example.

hook_context_ui_define()
  Provides an array of exported context definitions. Allows you
  to provide default contexts in your modules.

hook_context_ui_default_contexts_alter()
  A drupal_alter() that acts on the collected array of default
  contexts before they are cached.

hook_context_ui_active_contexts_alter()
  A drupal_alter() that acts on the collected array of active
  contexts on a given page load.

hook_context_ui_node_links_alter()
  A drupal_alter() that acts on the a links array of node
  creation and other contextual links for a given page load.

Maintainers
-----------
yhahn (Young Hahn)
jmiiccolis (Jeff Miccolis)

Contributors
------------
Pasqualle

