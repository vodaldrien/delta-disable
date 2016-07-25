<?php
/**
 * @file
 * Module settings admin file.
 */

/**
 * Callback for admin/structure/vm_delta_disable.
 *
 * Main settings form.
 */
function vm_delta_disable_settings_form($form, $form_state) {
  $entities = entity_get_info();

  foreach ($entities as $ekey => $entity) {
    $collapsed_entity_type = TRUE;
    $form[$ekey] = array(
      '#type' => 'fieldset',
      '#title' => $entity['label'],
      '#collapsed' => &$collapsed_entity_type,
      '#collapsible' => TRUE,
    );

    foreach ($entity['bundles'] as $bkey => $bundle) {
      $collapsed_bundle = TRUE;
      $form[$ekey][$bkey] = array(
        '#type' => 'fieldset',
        '#title' => $bundle['label'],
        '#collapsed' => &$collapsed_bundle,
        '#collapsible' => TRUE,
      );

      $active_entitybundle = variable_get('vm_delta_disable__' . $ekey . $bkey . '_state', 0);
      // Active state.
      $form[$ekey][$bkey]['vm_delta_disable__' . $ekey . $bkey . '_state'] = array(
        '#type' => 'checkbox',
        '#title' => t('Active'),
        '#default_value' => $active_entitybundle,
      );

      // Field information.
      $options = array();
      $fields = field_info_instances($ekey, $bkey);
      array_walk($fields, function($field, $fkey) use (&$options) {
        $options[$fkey] = $field['label'];
      });

      $enabled_fields = variable_get('vm_delta_disable__' . $ekey . $bkey . '_fields', array());
      $form[$ekey][$bkey]['vm_delta_disable__' . $ekey . $bkey . '_fields'] = array(
        '#type' => 'select',
        '#multiple' => TRUE,
        '#title' => t('Enabled Fields'),
        '#options' => $options,
        '#default_value' => $enabled_fields,
      );

      if ($active_entitybundle === 1) {
        $collapsed_entity_type = FALSE;
        $collapsed_bundle = FALSE;
      }
      unset($collapsed_bundle);
    }
    unset($collapsed_entity_type);

    $form['hr' . $ekey] = array(
      '#type' => 'markup',
      '#markup' => '<hr>',
    );
  }
  $form['#submit'][] = 'vm_delta_disable_clean_form_db';

  return system_settings_form($form);
}

/**
 * Submit handler for vm_delta_disable_settings_form().
 */
function vm_delta_disable_clean_form_db(&$form, &$form_state) {

  foreach ($form_state['values'] as $val_key => $val) {
    if (!$val) {
      variable_del($val_key);
      unset($form_state['values'][$val_key]);
    }
  }

}

/**
 * Callback for admin/structure/vm_delta_disable/view.
 */
function vm_delta_disable_view_disabled($form, $form_state) {
  $table_header = array(
    'id' => array('data' => '#', 'field' => 'id'),
    'entity_type' => array('data' => t('Entity Type'), 'field' => 'entity_type', 'sort' => 'asc'),
    'bundle' => array('data' => t('Bundle'), 'field' => 'bundle ', 'sort' => 'asc'),
    'entity_title' => array('data' => t('Entity')),
    'field_name' => array('data' => t('Field'), 'field' => 'field_name', 'sort' => 'asc'),
    'deltas' => array('data' => t('Deltas Disabled')),
    'actions' => array('data' => t('Actions')),
  );

  $disabled = db_select('vm_delta_disable', 'vmdd')->extend('TableSort')->extend('PagerDefault');
  $disabled->fields('vmdd');
  $disabled->limit(2);
  $disabled->orderByHeader($table_header);
  $results = $disabled->execute()->fetchAll();

  $rows = array();
  if ($results) {
    foreach ($results as $dsb_fld) {
      $deltas = unserialize($dsb_fld->data);
      $deltas = array_map(function($val) {
        return ++$val;
      }, $deltas);
      $del_link = l(t('Enable deltas'),
        'admin/structure/vm_delta_disable/enable/' . $dsb_fld->id, array(
          'query' => array('destination' => 'admin/structure/vm_delta_disable/view'),
        )
      );

      $e_info = entity_get_info($dsb_fld->entity_type);
      $f_info = field_info_instance($dsb_fld->entity_type, $dsb_fld->field_name, $dsb_fld->bundle);

      $entity = entity_load($dsb_fld->entity_type, array($dsb_fld->entity_id));
      $entity = current($entity);
      $entity_uri = entity_uri($dsb_fld->entity_type, $entity);
      $entity_label = l(entity_label($dsb_fld->entity_type, $entity), $entity_uri['path']);

      $rows[] = array(
        'id' => array('data' => $dsb_fld->id, 'title' => t('Disable action ID')),
        'entity_type' => array('data' => $e_info['label']),
        'bundle' => array('data' => $e_info['bundles'][$dsb_fld->bundle]['label']),
        'entity_title' => array('data' => $entity_label),
        'field_name' => array('data' => $f_info['label']),
        'deltas' => array('data' => implode(', ', $deltas)),
        'actions' => array('data' => $del_link),
      );
    }
  }

  $form['infodelta'] = array(
    '#type' => 'markup',
    '#markup' => '<sup>' . t('Delta numbers start from 1') . '</sup>',
  );
  $form['table'] = array(
    '#theme' => 'table',
    '#header' => $table_header,
    '#rows' => $rows,
    '#empty' => t('No disabled field deltas found.'),
  );
  $form['pager'] = array(
    '#type' => 'markup',
    '#markup' => theme('pager'),

  );

  return $form;
}


/**
 * Callback for admin/structure/vm_delta_disable/enable/% .
 */
function vm_delta_disable_remove_disable($form, &$form_state, $disabled_id) {
  $form['_disabled_id'] = array(
    '#type' => 'value',
    '#value' => $disabled_id,
  );
  $form['confirm_text'] = array(
    '#type' => 'markup',
    '#markup' => '<h3>' . t('Are you sure you want to proceed?') . '</h3>',
  );
  $form['submit_yes'] = array(
    '#type' => 'submit',
    '#value' => t('Yes'),
  );
  $form['submit_no'] = array(
    '#type' => 'submit',
    '#value' => t('No'),
  );
  return $form;
}

/**
 * Submit handler for vm_delta_disable_remove_disable().
 */
function vm_delta_disable_remove_disable_submit($form, $form_state) {
  if ($form_state['triggering_element']['#id'] !== 'edit-submit-yes') {
    drupal_set_message(t('Nothing modified'));
    return;
  }
  $disabled_id = $form['_disabled_id']['#value'];
  if (!$disabled_id) {
    drupal_set_message(t('An error occurred, nothing was modified.'), 'error');
    return;
  }

  $deleted = db_delete('vm_delta_disable')
    ->condition('id', $disabled_id, '=')
    ->execute();
  if ($deleted) {
    drupal_set_message(t('Deltas enabled successfully.'));
  }
  else {
    drupal_set_message(t('An error occurred, nothing was modified.'), 'error');
  }
}
