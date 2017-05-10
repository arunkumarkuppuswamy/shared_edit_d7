<?php
/**
 * @file
 * The api for shared edit.
 */

/**
 * Check if a user has access to edit a node.
 *
 * @param $node
 *   A node object or the nid for a node.
 * @param $account (optional)
 *   A user account or a uid for a user.
 * @param $reset
 *   Set to TRUE to clear the static cache.
 * return bool
 *   TRUE if the user has access to edit a node.
 *   FALSE if a user does not have access to edit a node.
 */
function shared_edit_access($node, $account = NULL, $reset = FALSE) {
  static $map = array();

  if ($reset) {
    $map = array();
    return;
  }

  if (is_int($node)) {
    $node = node_load($node);
  }

  if (is_null($account)) {
    global $user;
    $account = $user;
  }
  else if (is_int($account)) {
    $account = user_load(array('uid' => $account));
  }

  if (isset($map[$account->uid][$node->nid])) {
    return $map[$account->uid][$node->nid];
  }

  if ($access = node_access('update', $node)) {
    $map[$account->uid][$node->nid] = $access;
  }
  else if (variable_get('shared_edit_'. $node->type, 0) && user_access('share_node_edit_access', $account)) {
    $access = db_query('SELECT count(*) FROM {shared_edit} WHERE nid = :nid AND uid = :uid', array(':nid' => $node->nid, ':uid' => $account->uid))->fetchField();
    $map[$account->uid][$node->nid] = (bool)$access;
  }
  else {
    // Default to FALSE
    $map[$account->uid][$node->nid] = FALSE;
  }

  return $map[$account->uid][$node->nid];
}

/**
 * Add a user with edit permission to a node.
 *
 * @param $nid
 *   The nid for the node the user will be able to edit.
 * @param $uid
 *   The uid for the user being given edit permission.
 * @return bool
 *   TRUE if adding the user was successful.
 *   FALSE if there was an error.
 */
function shared_edit_add($nid, $uid) {
  // Make sure this is a valid node type to add a user to and that the user
  // has permission to have share edit access.
  $node = node_load($nid);
  $account = user_load($uid);
  if (variable_get('shared_edit_'. $node->type, 0) && user_access('share_node_edit_access', $account)) {
    // There is a unique key for the nid to uid pair. We do not need to check if it already
    // exists.
    // $result = (bool)db_query("INSERT INTO {shared_edit} (nid, uid) VALUES (%d, %d)", $nid, $uid);
    db_insert('shared_edit')
      ->fields(array(
        'nid' => $nid,
        'uid' => $uid,
      ))
      ->execute();
    $result = db_query('SELECT count(*) FROM {shared_edit} WHERE nid = :nid AND uid = :uid', array(':nid' => $nid, ':uid' => $uid))->fetchField();
    if (!$result) {
      // Record the unknown error.
      watchdog('shared_edit', 'The user %uid could not be given edit access to node %nid.', array('%uid' => $uid, '%nid' => $nid), WATCHDOG_NOTICE);
    }
    return $result;
  }
  else {
    watchdog('shared_edit', 'The user %uid could not be given edit access to node %nid. Either the user does not have access or shared edit is not enabled on this node type.', array('%uid' => $uid, '%nid' => $nid), WATCHDOG_ERROR);
    return FALSE;
  }
}

/**
 * Remove a users edit permission on a node.
 *
 * @param $nid
 *   The nid for the node the user will not be able to edit.
 * @param $uid
 *   The uid for the user having edit permission removed.
 * @return bool
 *   TRUE if adding the user was successful.
 *   FALSE if there was an error.
 */
function shared_edit_remove($nid, $uid) {
  $result = db_delete('shared_edit')
    ->condition('nid', $nid)
    ->condition('uid', $uid)
    ->execute();
  if (!$result) {
    watchdog('shared_edit', 'The user %uid could not have edit access removed from node %nid.', array('%uid' => $uid, '%nid' => $nid), WATCHDOG_NOTICE);
  }
  return $result;
}

/**
 * Remove all shared edit users from a node.
 *
 * @param $nid
 *   The nid for the node the users will not be able to edit.
 * @return bool
 *   TRUE if adding the user was successful.
 *   FALSE if there was an error.
 */
function shared_edit_remove_all($nid) {
  $result = db_delete('shared_edit')
    ->condition('nid', $nid)
    ->execute();
  if (!$result) {
    watchdog('shared_edit', 'The users with shared edit access on node %nid could not be removed.', array('%uid' => $uid, '%nid' => $nid), WATCHDOG_NOTICE);
  }
  return $result;
}

/**
 * Retrieve a list of users with shared edit access on a given node.
 *
 * @param $nid
 *   The nid for the node to get the users from.
 * @param $reset
 *   Set to TRUE to clear the cache.
 * @return array
 *   A list of users who have shared edit access on a node.
 */
function shared_edit_list_users($nid = null, $reset = FALSE) {
  static $map = array();

  if ($reset) {
    $map = array();
    return;
  }

  // Already have the user map cached so we return it.
  if (isset($map[$nid])) {
    return $map[$nid];
  }

  // We do not have the user map so look it up.
  $result = db_query('SELECT uid FROM {shared_edit} WHERE nid = :nid', array(":nid" => $nid))->fetchAll();
  $return = array();
  foreach ($result as $value) {
    $return[] = $value->uid;
  }
  $map[$nid] = $return;
  return $map[$nid];
}

/**
 * Retrieve a list of nodes a user has shared edit access on.
 *
 * @param $uid
 *   The uid for the user.
 * @param $reset
 *   Set to TRUE to clear the cache.
 * @return array
 *   A list of nodes a user has shared edit access on.
 */
function shared_edit_list_nodes($uid = null, $reset = FALSE) {
  static $map = array();

  if ($reset) {
    $map = array();
    return;
  }

  // Already have the user map cached so we return it.
  if (isset($map[$uid])) {
    return $map[$uid];
  }

  // We do not have the node map so look it up.
  $result = db_query('SELECT nid FROM {shared_edit} WHERE uid = :uid', array(":uid" => $uid))->fetchAll();
  $return = array();
  foreach ($result as $value) {
    $return[] = $value->nid;
  }
  $map[$uid] = $return;
  return $map[$uid];
}