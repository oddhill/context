$Id$

context_prefix for Drupal 6.x

Installation
------------
context_prefix can be installed like any other Drupal module -- place
it in the modules directory for your site and enable it (and its
requirement, context) on the admin/build/modules page.

Basic usage
-----------
context_prefix is an API module -- it is meant to be a helper (and one
that does some heavy lifting at that!) for other modules interested in
using path prefixing to sustain information between pages without
using a SESSION or other hackish means.

Translation: context_prefix does absolutely nothing for the end user
out of the box without other modules that take advantage of its API.

Usage overview
--------------
The basic task that context_prefix fulfills is sustaining some piece of
information that your module is interested in between page loads. It
does this by adding this information via a 'prefix' onto all internal
links on a given page load and then parsing this prefix into something
usable for your module on subsequent page loads.

Example:

Your user would like to view the spanish version of the site. You
provide a "Spanish" link at the top of your page which points to
yoursite.com/es. Once she clicks this link, all urls on the page are
prefixed with 'es' (e.g. 'es/node/43', 'es/taxonomy/term', even
'es/admin'). On subsequent page loads, the 'es' prefix is parsed and
passed on to the Internationalization module so that it knows to
display the spanish version of your content. The prefix 'es' is hidden
from all other Drupal modules which continue behaving as if the user is
visiting a normal Drupal page url ('node/43', 'taxonomy/term', and
'admin').

API
---
There are several required integration points for your module before
you can realistically get some prefixing working.

Overview:

1. Register your module as a prefix provider using
   hook_context_prefix_provider().

2. Implement some method of providing valid prefixes either by entering
   them into the database using context_prefix_form() and
   context_prefix_api() or providing them programatically via
   hook_context_prefix_prefixes().

3. Implement the reactive behavior you would like in your module's
   callback described in hook_context_prefix_provider().

The details:

1. Your module registers itself as a prefix provider via
   hook_context_prefix_provider().

2. Your module allows the user to enter a prefix to associate with a
   certain organic group (e.g. group nid 43 = 'knitting').
   NOTE: spaces/spaces_og actually provides this functionality.

3. On hook_init(), context_prefix_init finds any registered prefixes
   and fires the associated provider's callback. In our example,
   context_prefix would pass to spaces_og's callback nid 43.

4. The provider gets a chance to take some actions via its callback
   (spaces_og sets the active group context and space to the knitting
   group).

5. Context_prefix rewrites all url's on the page to contain the
   'knitting' prefix.
   NOTE: certain links can be excluded from prefixing by using the
   'unprefix' parameter in the $options array passed to l().

Maintainers
-----------
yhahn (Young Hahn)
jmiccolis (Jeff Miccolis)
