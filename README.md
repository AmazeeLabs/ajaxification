# Ajaxification

The module allows ajaxify any part of a Drupal site. It handles link clicks and
form submits.

## Configuration

Visit `admin/config/user-interface/ajaxification` path after the module installation.

## Message support
If you want Drupal messages support: install 
[jGrowl](https://github.com/stanlemon/jGrowl) at
`sites/all/libraries/jgrowl`:

    git submodule add git@github.com:stanlemon/jGrowl.git sites/all/libraries/jgrowl
    
# The following info is outdated!

## hook_ajaxification_configurations()

The module does nothing by itself. Instead, it provides the
`ajaxification_configurations` hook, which other modules use to describe what
they want to be ajaxified.

The hook is described in [ajaxification.api.php](./ajaxification.api.php).

## ajaxification.js operation logic (developer cheatsheet)

There are three entry points. Each of them has a different workflow, but there
are common actions:

    savePageState(url, data = null) saves the following in the "history" object:
      - URL
      - Drupal.settings
      - $replaceWrapper.html()
      - head > title
      - h1 (if only one on the page)
      - body class attribute
    (if "data" provided, it tries get everything from it)

    restorePageState(url)
      - restores from the "history" object:
        - Drupal.settings
        - $replaceWrapper contents
        - head > title
        - h1 (if it was saved, and if currently only one h1 presents on the page)
        - body class attribute
      - applies Drupal behaviours on $replaceWrapper
      - if (trackPageViewWithGA): tacks page view with Google Analytics

### The first full page load

    - find first configuration where ($(selectors.replaceWrapperContext + ' ' + selectors.replaceWrapper) > 0)
    - if (no proper configuration was found): break execution
    - if (historyApiOnly && browser does not support History API): break execution
    - savePageState()

### Link click or for submit (doAjax())

These event are handled only if link/form is located inside ($ajaxRegions ||
replaceWrapper) and href/action starts with on of basePaths.

    - breakAjaxRequest()
    - show progress indicator
    - send request to the server, on success:
      - calls window.history.pushState() is possible
      - execute ajax_commands provided by the ajaxification module, this includes:
        - extend Drupal.setting with new settings
        - load new css/js files
      - savePageState() providing data from the response
      - restorePageState()
      - hide progress indicator
      - scroll to selectors.scrollTo (if any)

### History navigation (on popstate event)

    - if (URL is not found in the "history" object): do nothing (normally, this should never happen)
    - breakAjaxRequest()
    - restore Drupal.settings from the "history" object
    - if (reloadOnHistoryWalk): imitate link click with the given URL
    - else: restorePageState()

## How Drupal AJAX is used

TBD

## TODO

1. Provide the default styles for the AJAX indicatior.
1. Merge body classes, but remove "page-*" before the merge.
1. Fill "How Drupal AJAX is used" section in the README.
1. Provide an admin UI which will simple implement the
`ajaxification_configurations` hook (save as `ajaxification_configuration_{id}` variable).
