$Id$

Context 3.0 for Drupal 6.x

Installation
------------
Context can be installed like any other Drupal module -- place it in
the modules directory for your site and enable it (and its requirement,
context) on the admin/build/modules page.

You will probably also want to install Context UI which provides a way for
you to edit contexts through the Drupal admin interface.

Upgrade from Context 2.0 for Drupal 6.x
---------------------------------------

- Download latest Context 3.0 and latest CTools release and place in modules
  directory.
- Run update.php.
- If your site contains contexts defined in code they will be overridden.
  Re-export them to code again.

Basic usage
-----------
Context allows you to manage contextual conditions and reactions for
different portions of your site. You can think of each context as
representing a "section" of your site. For each context, you can choose
the conditions that trigger this context to be active and choose different
aspects of Drupal that should respond to this active context.

Think of conditions as a set of rules that are checked during page load
to see what context is active. Any reactions that are associated with
active contexts are then fired.

Example
-------
You want to create a "pressroom" section of your site. You have a press
room view that displays press release nodes, but you also want to tie
a book with media resources tightly to this section. You would also
like a contact block you've made to appear whenever a user is in the
pressroom section.

1. Add a new context on admin/build/context
2. Under "Conditions", associate the pressroom nodetype, the pressroom view,
   and the media kit book with the context.
3. Under "Reactions > Menu", choose the pressroom menu item to be set active.
4. Under "Reactions > Blocks", add the contact block to a region.
5. Save the context.

Hooks
-----
See context.api.php for the hooks made available by context.

Maintainers
-----------
yhahn (Young Hahn)
jmiccolis (Jeff Miccolis)

Contributors
------------
alex_b (Alex Barth)
dmitrig01 (Dmitri Gaskin)
Pasqualle (Csuthy Bálint)
