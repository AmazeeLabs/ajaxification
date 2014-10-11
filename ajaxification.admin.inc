<?php

//AXXX
function ajaxification_configurations_form($form, &$form_state) {
  //AXXX add common description
  //AXXX add descriptions for each fields
  foreach (ajaxification_ajaxification_configurations() + array('' => array()) as $id => $cfg) {
    $key = 'ajaxification_configuration_' . $id;
    $form[$key] = array(
      '#type' => 'fieldset',
      '#title' => $id === '' ? t('Add new configuration') : $id,
      '#collapsible' => TRUE,
      '#collapsed' => $id !== '',
        '#tree' => TRUE,
    );
    $form[$key]['id'] = array(
      '#type' => 'textfield',
      '#title' => t('ID'),
      '#maxlength' => 100,
      '#default_value' => $id,
    );
    $form[$key]['basePaths'] = array(
      '#type' => 'textarea',
      '#title' => t('Base paths'),
      '#default_value' => isset($cfg['basePaths']) ? implode("\n", $cfg['basePaths']) : NULL,
    );
    $form[$key]['selectors'] = array(
      '#type' => 'fieldset',
      '#title' => t('Selectors'),
      '#tree' => TRUE,
    );
    $form[$key]['selectors']['replaceWrapper'] = array(
      '#type' => 'textfield',
      '#title' => t('Replace wrapper'),
      '#default_value' => isset($cfg['selectors']['replaceWrapper']) ? $cfg['selectors']['replaceWrapper'] : NULL,
    );
    $form[$key]['selectors']['replaceWrapperContext'] = array(
      '#type' => 'textfield',
      '#title' => t('Replace wrapper context'),
      '#default_value' => isset($cfg['selectors']['replaceWrapperContext']) ? $cfg['selectors']['replaceWrapperContext'] : NULL,
    );
    $form[$key]['selectors']['triggerRegions'] = array(
      '#type' => 'textfield',
      '#title' => t('Trigger regions'),
      '#default_value' => isset($cfg['selectors']['triggerRegions']) ? $cfg['selectors']['triggerRegions'] : NULL,
    );
    $form[$key]['reloadOnHistoryWalk'] = array(
      '#type' => 'checkbox',
      '#title' => t('Reload on history walk'),
      '#default_value' => isset($cfg['reloadOnHistoryWalk']) ? $cfg['reloadOnHistoryWalk'] : NULL,
    );
    $form[$key]['trackPageViewWithGA'] = array(
      '#type' => 'checkbox',
      '#title' => t('Track page view with Google Analytics'),
      '#default_value' => isset($cfg['trackPageViewWithGA']) ? $cfg['trackPageViewWithGA'] : NULL,
    );
  }
  $form['#submit'][] = 'ajaxification_configurations_form_submit';
  return system_settings_form($form);
}

//AXXX
function ajaxification_configurations_form_validate(&$form, &$form_state) {
  foreach ($form_state['values'] as $key => $cfg) {
    if (strpos($key, 'ajaxification_configuration_') === 0) {
      $cfg['id'] = trim($cfg['id']);
      $cfg['basePaths'] = array_filter(explode("\n", $cfg['basePaths'])) ?: array('');
      unset($form_state['values'][$key]);
      if ($cfg['id'] !== '') {
        $form_state['values']['ajaxification_configuration_' . $cfg['id']] = $cfg;
      }
    }
  }
}

//AXXX
function ajaxification_configurations_form_submit(&$form, &$form_state) {
  $cfg_ids = array();
  foreach ($form_state['values'] as $key => $cfg) {
    if (strpos($key, 'ajaxification_configuration_') === 0) {
      $cfg_ids[] = $form_state['values'][$key]['id'];
    }
  }
  $query = db_delete('variable');
  $query->condition('name', 'ajaxification_configuration_%', 'LIKE');
  if (!empty($cfg_ids)) {
    $query->condition('name', $cfg_ids, 'NOT IN');
  }
  $query->execute();
}
