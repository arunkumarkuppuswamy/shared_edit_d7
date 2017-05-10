<?php

/**
 * @file
 * Contains the page callbacks for shared edit.
 */

/**
 * Callback: Shared Edit add/remove form.
 */
function shared_edit_access_form($form, &$form_state) {
  $form = array();
  $nid = arg(1);
  $node = node_load($nid);
  $result = db_query('SELECT se.uid, u.name FROM {shared_edit} se LEFT JOIN {users} u ON se.uid = u.uid WHERE se.nid = :nid', array(":nid" => $node->nid))->fetchAll();
  $form['users']['user_'. $node->uid]['name'] = array('#value' => l($node->title, 'user/'. $node->uid));
  $form['users']['user_'. $node->uid]['operation'] = array('#value' => '<em>'. t('locked') .'</em>');
  $header = array('name' => 'Name', 'operation' => 'Manage');
  $rows = array();
  foreach ($result as $value) {
    $account = user_load($value->uid);
    if ($value->uid != $node->uid) {
      $rows[] = array(
        'name' => l($value->name, 'user/'. $value->uid),
        'operation' => l(t('delete'), 'node/'. $nid .'/shared_edit/remove/'. $value->uid),
      );
    }
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $form['user_list'] = array(
    '#type' => 'markup',
    '#markup' => $output,
  );
  $form['shared_edit_add_new'] = array(
    '#id' => 'shared_edit_add_new',
    '#type' => 'textfield',
    '#size' => 30,
    '#maxlength' => 255,
    '#required' => TRUE,
    '#autocomplete_path' => 'user/autocomplete',
  );
  $form['shared_edit_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add access'),
  );

  return $form;
}


function shared_edit_access_form_validate($form, &$form_state) {
  $name = $form_state['values']['shared_edit_add_new'];
  $node = node_load(arg(1));
  
  $account = user_load_by_name($name);
  if ($node->name == $name) {
    form_set_error('shared_edit_add_new', t('The username is already the content owner.'));
  }
  else if (!empty($account) && db_query('SELECT count(*) FROM {shared_edit} WHERE nid = :nid AND uid = :uid', array(":nid" => $node->nid, ":uid" => $account->uid))->fetchField()) {
    form_set_error('shared_edit_add_new', t('The username already has edit access to the content.'));
  }
  else if (empty($account)) {
    form_set_error('shared_edit_add_new', t('Please enter a valid username.'));
  }
}

function shared_edit_access_form_submit($form, &$form_state) {
  $account = user_load_by_name($form_state['values']['shared_edit_add_new']);
  $node = node_load(arg(1));
  if (!shared_edit_add($node->nid, $account->uid)) {
    // There was an error. Throw a message.
    drupal_set_message(t('The user %user could not be added. Please see a website administrator for more details.', array('%user' => $account->name)), 'error');
  }
}

/**
 * Menu callback: remove a users access to edit a node.
 */
function shared_edit_remove_user() {
  $node = node_load(arg(1));
  $account = user_load(arg(4));
  if (!shared_edit_remove($node->nid, $account->uid)) {
    // There was an error. Throw a message.
    drupal_set_message(t('The user %user could not be removed. Please see a website administrator for more details.', array('%user' => $account->name)), 'error');
  }
  drupal_goto('node/'. $node->nid .'/shared_edit');
}