<?php

/**
 * Page callback: configurations overview form.
 */
function ajaxification_configurations_page() {
  // @todo (alex): maybe add weights and tabledrag?
  $configurations = _ajaxification_get_configurations();
  $header = array(
    t('Identifier'),
    t('Description'),
    t('Operations'),
  );
  $rows = array();
  foreach ($configurations as $cfg) {
    $rows[] = array(
      $cfg['id'],
      $cfg['description'],
      theme('links', array(
        'links' => array(
          array(
            'title' => t('Edit'),
            'href' => 'admin/config/user-interface/ajaxification/configurations/edit/' . $cfg['id'],
          ),
          array(
            'title' => t('Export'),
            'href' => 'admin/config/user-interface/ajaxification/configurations/export/' . $cfg['id'],
          ),
          array(
            'title' => t('Delete'),
            'href' => 'admin/config/user-interface/ajaxification/configurations/delete/' . $cfg['id'],
          ),
        ),
        'attributes' => array(
          'class' => array('links', 'inline'),
        ),
      )),
    );
  }
  return theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('Add one ore more configurations to make Ajaxification work.'),
  ));
}

/**
 * Form callback: ajaxification configuration.
 */
function ajaxification_configuration_form($form, &$form_state, $cfg = NULL) {
  $form['explanations'] = array(
    '#type' => 'fieldset',
    '#title' => t('Explanations'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    'common' => array(
      '#markup' => t('When any site page is loaded (without AJAX), the JavaScript code of the Ajaxification module will go though all defined configurations and test if one of them could be used. The following checks happen:<ol><li>If "History API only" flag is set and user browser does not support History API: skip configuration.</li><li>If current page URL does not start with one of the "Base paths": skip configuration.</li><li>Search for the "Replace wrapper" (which is wrapped with "Replace wrapper context", if set). If not found: skip configuration.</li></ol>The first configuration that passed these checks, sets as active one.'),
    ),
  );
  $form['id'] = array(
    '#type' => 'textfield',
    '#title' => t('Identifier'),
    '#description' => t('Machine name. Use only letters, numbers, and underscore symbols.'),
    '#maxlength' => 100,
    '#required' => TRUE,
    '#default_value' => isset($cfg['id']) ? $cfg['id'] : NULL,
  );
  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Administrative description'),
    '#description' => t('Description that is only displayed on the configurations overview page.'),
    '#default_value' => isset($cfg['description']) ? $cfg['description'] : NULL,
  );
  $form['basePaths'] = array(
    '#type' => 'textarea',
    '#title' => t('Base paths'),
    '#description' => t('Drupal internal paths. One per line.<br />The module will check current page URL to determine the active configuration (see the Explanations section).<br />All processes links and forms will be tested to start with one of the provided base path of the active configuration.<br />For example, if you set "search/node" as a base path, the following paths will pass check: "search/node/test", "search/node", "search/node?key=val"; and the following ones will not pass: "search/nodes", "search".'),
    '#default_value' => isset($cfg['basePaths']) ? implode("\n", $cfg['basePaths']) : NULL,
  );
  $form['selectors'] = array(
    '#type' => 'fieldset',
    '#title' => t('Selectors'),
    '#description' => t('jQuery selectors.'),
    '#tree' => TRUE,
  );
  $form['selectors']['replaceWrapper'] = array(
    '#type' => 'textfield',
    '#title' => t('Replace wrapper'),
    '#description' => t('The main selector defining the region to be replaced by the AJAX call. Should return one element.'),
    '#required' => TRUE,
    '#default_value' => isset($cfg['selectors']['replaceWrapper']) ? $cfg['selectors']['replaceWrapper'] : NULL,
  );
  $form['selectors']['replaceWrapperContext'] = array(
    '#type' => 'textfield',
    '#title' => t('Replace wrapper context'),
    '#description' => t('Used to specify the "Replace wrapper" surrounding. For example: "body.node-type-article". Used only for detecting active configuration (see the Explanations section).'),
    '#default_value' => isset($cfg['selectors']['replaceWrapperContext']) ? $cfg['selectors']['replaceWrapperContext'] : NULL,
  );
  $form['selectors']['triggerRegions'] = array(
    '#type' => 'textfield',
    '#title' => t('Trigger regions'),
    '#description' => t('Defines one or more areas inside the "Replace wrapper" where link clicks and form submits should be processed with AJAX. If omitted, the whole "Replace wrapper" is used.'),
    '#default_value' => isset($cfg['selectors']['triggerRegions']) ? $cfg['selectors']['triggerRegions'] : NULL,
    '#maxlength' => 512,
  );
  $form['selectors']['scrollTo'] = array(
    '#type' => 'textfield',
    '#title' => t('Scroll to'),
    '#description' => t('The window top will be scrolled to the element defined by this selector after the page content updated.'),
    '#default_value' => isset($cfg['selectors']['scrollTo']) ? $cfg['selectors']['scrollTo'] : NULL,
  );
  $form['selectors']['scrollToOffsetElement'] = array(
    '#type' => 'textfield',
    '#title' => t('Scroll to: offset element'),
    '#description' => t('If you have a sticky header with "position: fixed", set here its selector. Example: .sticky-wrapper.is-sticky'),
    '#default_value' => isset($cfg['selectors']['scrollToOffsetElement']) ? $cfg['selectors']['scrollToOffsetElement'] : NULL,
    '#states' => array(
      'visible' => array(
        ':input[name="selectors[scrollTo]"]' => array('empty' => FALSE),
      ),
    ),
  );
  $form['historyApiOnly'] = array(
    '#type' => 'checkbox',
    '#title' => t('History API only'),
    '#description' => t('History API is a browser feature allowing change URL path without page reload. If this flag is not set and user browser does not support History API: URLs won\'t be updated during AJAX navigation.'),
    '#default_value' => isset($cfg['historyApiOnly']) ? $cfg['historyApiOnly'] : NULL,
  );
  $form['reloadOnHistoryWalk'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reload on history walk'),
    '#description' => t('Force content reload when user clicks Back browser button.'),
    '#default_value' => isset($cfg['reloadOnHistoryWalk']) ? $cfg['reloadOnHistoryWalk'] : NULL,
  );
  $form['trackPageViewWithGA'] = array(
    '#type' => 'checkbox',
    '#title' => t('Track page view with Google Analytics'),
    '#description' => t('If this flag is set and you have google Analytics installed, AJAX navigation will send pageView events to GA.'),
    '#default_value' => isset($cfg['trackPageViewWithGA']) ? $cfg['trackPageViewWithGA'] : NULL,
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  if (isset($cfg['id'])) {
    $form['actions']['delete'] = array(
      '#markup' => l(t('Delete'), 'admin/config/user-interface/ajaxification/configurations/delete/' . $cfg['id'], array(
        'attributes' => array(
          'style' => 'margin-right: 12px;',
        ),
      )),
    );
  }
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/config/user-interface/ajaxification/configurations'),
  );
  return $form;
}

/**
 * Form validate callback: ajaxification configuration.
 */
function ajaxification_configuration_form_validate(&$form, &$form_state) {
  $form_state['values']['basePaths'] = array_filter(array_map('trim', explode("\n", $form_state['values']['basePaths']))) ?: array('');
  $form_state['values'] = _ajaxification_validate_cfg($form_state['values'], $form);
}

/**
 * Form submit callback: ajaxification configuration.
 */
function ajaxification_configuration_form_submit(&$form, &$form_state) {
  if ($form['id']['#default_value'] != $form_state['values']['id']) {
    variable_del('ajaxification_configuration_' . $form['id']['#default_value']);
  }
  form_state_values_clean($form_state);
  variable_set('ajaxification_configuration_' . $form_state['values']['id'], $form_state['values']);
  drupal_set_message(t('The configuration options have been saved.'));
  $form_state['redirect'] = 'admin/config/user-interface/ajaxification/configurations';
}

/**
 * Form callback: delete ajaxification configuration.
 */
function ajaxification_configuration_delete_form($form, &$form_state, $cfg) {
  return confirm_form(
    array(
      'cfg_id' => array(
        '#type' => 'value',
        '#value' => $cfg['id'],
      ),
    ),
    t('Are you sure you want to delete %cfg configuration?', array('%cfg' => $cfg['id'])),
    'admin/config/user-interface/ajaxification/configurations',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Form submit callback: delete ajaxification configuration.
 */
function ajaxification_configuration_delete_form_submit(&$form, &$form_state) {
  variable_del('ajaxification_configuration_' . $form_state['values']['cfg_id']);
  drupal_set_message(t('The %cfg configuration has been deleted.', array('%cfg' => $form_state['values']['cfg_id'])));
  $form_state['redirect'] = 'admin/config/user-interface/ajaxification/configurations';
}

/**
 * Form callback: export ajaxification configuration.
 */
function ajaxification_configuration_export_form($form, &$form_state, $cfg) {
  $form['export'] = array(
    '#type' => 'textarea',
    '#title' => '',
    '#default_value' => function_exists('entity_var_json_export') ? entity_var_json_export($cfg) : drupal_json_encode($cfg),
    '#attributes' => array(
      'onclick' => 'this.select(); this.onclick=null;',
    ),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['back']['#markup'] = l(t('Return to the configurations list'), 'admin/config/user-interface/ajaxification/configurations');
  return $form;
}

/**
 * Form callback: import ajaxification configuration.
 */
function ajaxification_configuration_import_form($form, &$form_state) {
  $form['import'] = array(
    '#type' => 'textarea',
    '#title' => t('Paste configuration exported JSON'),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['actions']['back']['#markup'] = l(t('Return to the configurations list'), 'admin/config/user-interface/ajaxification/configurations');
  return $form;
}

/**
 * Form validate callback: import ajaxification configuration.
 */
function ajaxification_configuration_import_form_validate(&$form, &$form_state) {
  $cfg = @drupal_json_decode($form_state['values']['import']);
  $form_state['values']['cfg'] = _ajaxification_validate_cfg($cfg);
  if (!$form_state['values']['cfg']) {
    form_error($form['import'], t('Invalid input'));
  }
}

/**
 * Form submit callback: import ajaxification configuration.
 */
function ajaxification_configuration_import_form_submit(&$form, &$form_state) {
  variable_set('ajaxification_configuration_' . $form_state['values']['cfg']['id'], $form_state['values']['cfg']);
  drupal_set_message(t('The configuration has been imported.'));
  $form_state['redirect'] = 'admin/config/user-interface/ajaxification/configurations/edit/' . $form_state['values']['cfg']['id'];
}

/**
 * Form submit callback: ajaxification settings.
 */
function ajaxification_settings_form($form, &$form_state) {
  // @todo: add jgrowl settings
  return $form;
}
