// $Id$

CONTEXT MODULE

The context module provides a very simple API for storing information about a
page as it is being generated. This allows modules and themes to be aware of
things that they otherwise would not be able to easily detect.

CONTEXT UI

Context UI provides a user interface for working with context. It allows you to
define what node types, views, user pages, etc cause a context to be sent and
what blocks and menu items respond to that context. One of the main uses of
Context UI is to handle block visibility, vastly simplifying the task of
managing blocks on a site.

Context UI also adds a new hook; hook_context_define(). This allows modules
authors to provide default context definitions. It is designed to use the
exported contexts from context UI. A context can defined in the UI, exported and
pasted into a module, very similarly to how exported views work.

CONTEXT PREFIX

Context prefix allows a URL 'prefix' to set a context. The module has two
different modes; 'path prefixes' and 'full domain'. Both modes set a context
based on a URL, but they pull for different places. The 'path prefix' mode uses
a string prepended to a Drupal URL, very similarly to i18n. If a prefix was
'foo' an path would look like 'foo/node/x'. Each time such a path was access it
would set the context that corresponds to 'foo'. 'Full domain' looks at the
domain being used to access the site, and correspondingly sets a context.

Context prefix uses custom_url_rewrite() to generate outgoing paths and handle
incoming paths with the proper prefix when in 'path prefix' mode. You may
experience unpredictable behavior if this function is used by another module
other than i18n. We've taken care to make sure we remain i18n compatible, but
uses have reported issues with other modules, like the OG_Vocab.

In order to provide links which leave context, Context Prefix provides cl().
This function is a wrapper around l() that takes on additional argument,
$dropcontext, which when set will generate links which leave the current context
prefix context.