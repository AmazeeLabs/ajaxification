<?php

/**
 * Alters static caches of the drupal_alter(), so ajaxification alter hook is
 * executed in the last order.
 *
 * The reason. Even if we have the hook_module_implements_alter(), the theme's
 * functions always ran in the last order, so we do the trick:
 * 1) initialize static caches of the drupal_alter() without actual alter call,
 * 2) reorder the static caches as we want.
 *
 * @see _ajaxification_init_drupal_alter()
 *
 * @param string $hook
 *   Alter hook name. Example: "js" for "hook_js_alter, etc.
 */
function _ajaxification_the_last_alter($hook) {
  $functions =& drupal_static('drupal_alter');
  _ajaxification_init_drupal_alter($hook);
  $function = "ajaxification_{$hook}_alter";
  if (($key = array_search($function, $functions[$hook])) !== FALSE) {
    unset($functions[$hook][$key]);
    $functions[$hook][] = $function;
  }
}

/**
 * This function is a copy of the drupal_alter(), with one difference: it does
 * not execute the alter function. It just initializes the function's static
 * caches, so it could be altered before the actual drupal_alter run.
 *
 * @see _ajaxification_the_last_alter()
 */
function _ajaxification_init_drupal_alter($type) {
  static $drupal_static_fast;
  if (!isset($drupal_static_fast)) {
    $drupal_static_fast['functions'] = &drupal_static('drupal_alter');
  }
  $functions = &$drupal_static_fast['functions'];
  if (is_array($type)) {
    $cid = implode(',', $type);
    $extra_types = $type;
    $type = array_shift($extra_types);
    if (empty($extra_types)) {
      unset($extra_types);
    }
  }
  else {
    $cid = $type;
  }
  if (!isset($functions[$cid])) {
    $functions[$cid] = array();
    $hook = $type . '_alter';
    $modules = module_implements($hook);
    if (!isset($extra_types)) {
      foreach ($modules as $module) {
        $functions[$cid][] = $module . '_' . $hook;
      }
    }
    else {
      $extra_modules = array();
      foreach ($extra_types as $extra_type) {
        $extra_modules = array_merge($extra_modules, module_implements($extra_type . '_alter'));
      }
      if (array_diff($extra_modules, $modules)) {
        $modules = array_intersect(module_list(), array_merge($modules, $extra_modules));
        $implementations = array_fill_keys($modules, FALSE);
        drupal_alter('module_implements', $implementations, $hook);
        $modules = array_keys($implementations);
      }
      foreach ($modules as $module) {
        $function = $module . '_' . $hook;
        if (function_exists($function)) {
          $functions[$cid][] = $function;
        }
        foreach ($extra_types as $extra_type) {
          $function = $module . '_' . $extra_type . '_alter';
          if (function_exists($function)) {
            $functions[$cid][] = $function;
          }
        }
      }
    }
    global $theme, $base_theme_info;
    if (isset($theme)) {
      $theme_keys = array();
      foreach ($base_theme_info as $base) {
        $theme_keys[] = $base->name;
      }
      $theme_keys[] = $theme;
      foreach ($theme_keys as $theme_key) {
        $function = $theme_key . '_' . $hook;
        if (function_exists($function)) {
          $functions[$cid][] = $function;
        }
        if (isset($extra_types)) {
          foreach ($extra_types as $extra_type) {
            $function = $theme_key . '_' . $extra_type . '_alter';
            if (function_exists($function)) {
              $functions[$cid][] = $function;
            }
          }
        }
      }
    }
  }
  // HACKED >>>
//  foreach ($functions[$cid] as $function) {
//    $function($data, $context1, $context2, $context3);
//  }
  // <<< HACKED
}

/**
 * This function is a replacement for ajax_render() until
 * https://drupal.org/node/208611#comment-5074398 is committed.
 *
 * We need this function because drupal_get_js() could be called twice, and some
 * JS settings become duplicated.
 *
 * Hacked places are marked with "HACKED" comments.
 *
 * @see ajax_render()
 */
function _ajaxification_ajax_render($commands = array()) {
  foreach (array('css', 'js') as $type) {
    if (empty($_POST['ajax_page_state'][$type])) {
      $items[$type] = array();
    }
    else {
      $function = 'drupal_add_' . $type;
      $items[$type] = $function();
      drupal_alter($type, $items[$type]);
      foreach ($items[$type] as $key => $item) {
        if (is_numeric($key)) {
          unset($items[$type][$key]);
        }
      }
      $items[$type] = array_diff_key($items[$type], $_POST['ajax_page_state'][$type]);
    }
  }
  if (isset($items['js']['settings'])) {
    unset($items['js']['settings']);
  }
  $styles = drupal_get_css($items['css'], TRUE);
  $scripts_footer = drupal_get_js('footer', $items['js'], TRUE);
  $scripts_header = drupal_get_js('header', $items['js'], TRUE);
  $extra_commands = array();
  if (!empty($styles)) {
    $extra_commands[] = ajax_command_prepend('head', $styles);
  }
  if (!empty($scripts_header)) {
    $extra_commands[] = ajax_command_prepend('head', $scripts_header);
  }
  if (!empty($scripts_footer)) {
    $extra_commands[] = ajax_command_append('body', $scripts_footer);
  }
  if (!empty($extra_commands)) {
    $commands = array_merge($extra_commands, $commands);
  }
  $scripts = drupal_add_js();
  if (!empty($scripts['settings'])) {
    $settings = $scripts['settings'];
    // HACKED >>>
//  array_unshift($commands, ajax_command_settings(call_user_func_array('array_merge_recursive', $settings['data']), TRUE));
    array_unshift($commands, ajax_command_settings(drupal_array_merge_deep_array($settings['data']), TRUE));
    // <<< HACKED
  }
  drupal_alter('ajax_render', $commands);
  return drupal_json_encode($commands);
}
