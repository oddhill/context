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

Menu integration
----------------
To enable "active" classes on your primary links menu when in a certain
context, you need to push your menu links through
theme_context_ui_links() instead of Drupal's regular theme_links().
This usually can be done very easily in a theme's template.php 
[theme]_preprocess_page hook by adding a line like the following:

  $vars['primary_links'] = theme('context_ui_links', menu_primary_links());

Maintainers
-----------
yhahn (Young Hahn)
jmiiccolis (Jeff Miccolis)

Contributors
------------
Pasqualle

