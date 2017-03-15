<?php
/**
 * Implement's Drupal's hook_form
 *
 * @param $form
 * @param $form_state
 * @param $galaxy_id
 *
 * @return $form
 */
function tripal_galaxy_admin_configuration_form($form, $form_state, $galaxy_id = NULL) {
  // set the breadcrumb
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Extensions', 'admin/tripal/extension');
  $breadcrumb[] = l('Galaxy', 'admin/tripal/extension/galaxy');
  drupal_set_breadcrumb($breadcrumb);

  $default_server_name = "";
  $default_user_name = "";
  $default_description = "";
  $default_api_key = "";
  $default_url = "";

  // If a galaxy id is set, feel free to declare all the default values with the
  // appropriate
  // Galaxy instance
  if ($galaxy_id != NULL) {
    $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
    $galaxy_instance = db_query($sql, array(':galaxy_id' => $galaxy_id));
    $galaxy_instance = $galaxy_instance->fetchObject();
    if ($galaxy_instance != NULL) {
      $default_api_key = $galaxy_instance->api_key;
      $default_description = $galaxy_instance->description;
      $default_server_name = $galaxy_instance->servername;
      $default_user_name = $galaxy_instance->username;
      $default_url = $galaxy_instance->url;
    }
  }

  // The galaxy id
  $form['galaxy_id'] = array(
    '#type' => 'value',
    '#value' => $galaxy_id
  );

  // Server Name
  $form['servername'] = array(
    '#title' => t('Galaxy Server Name'),
    '#type' => 'textfield', '#size' => 50,
    '#default_value' => $default_server_name,
    '#description' => 'Please provide the name of the remote Galaxy Server',
    '#required' => TRUE
  );

  // Description
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $default_description,
    '#description' => 'Provide any additional details about this server.',
    '#required' => FALSE
  );

  // URL
  $form['url'] = array(
    '#title' => t('URL'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_url,
    '#description' => 'The URL for the remote Galaxy server.',
    '#required' => TRUE
  );

  // User Name
  $form['username'] = array('#title' => t('User Name'), '#type' => 'textfield', '#size' => 50,
    '#default_value' => $default_user_name,
    '#description' => 'The user name for the Galaxy server.
        This username is used to launch all jobs by default.
        If this field is left blank then it is expected that the user
        has an account on the Galaxy server and will provide their username
        when executing workflows.', '#required' => FALSE);

  // API Key
  $form['api_key'] = array('#title' => t('API Key'), '#type' => 'textfield', '#size' => 50,
    '#default_value' => $default_api_key,
    '#description' => "The API key for the user name specified above.
        If this field is left blank then it is expected that the user
        will provide their own API key when submitting a job.", '#required' => FALSE);

  // Default submit
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' =>
    'submit',
    '#value' => t('Submit')
  );

  // Test Connection
  $form['test_connect'] = array(
    '#type' => 'submit',
    '#name' => 'test_connect',
    '#value' => t('Test Connection')
  );

  // Delete
  if ($galaxy_id) {
    $form['delete'] = array(
      '#type' => 'submit',
      '#name' => 'delete',
      '#value' => t('Delete')
    );
  }

  return $form;
}

/**
 * Custom Validate for hook_form
 *
 * @param
 *          $form
 * @param
 *          $form_state
 * @return boolean Return true if the form is valid for deletion, false
 *         otherwise
 */
function tripal_galaxy_admin_configuration_form_delete_validate($form, &$form_state) {

  // Check to see if we are deleting an instance.
  if ($form_state['clicked_button']['#name'] == 'delete') {
    $sql = "SELECT * FROM {tripal_galaxy} WHERE servername=" . $form['tripal_galaxy_server_name']['#value'];
    $results = db_result(db_query($sql));
    if ($results == NULL || $results == FALSE) {

      // If there is no entry with this servername return False.
      form_set_error('servername', t('A galaxy instance with that servername does not exist'));
      return FALSE;
    }
  }
}

/**
 * Implements Drupal's hook_form_submit
 * Handles submitting a new galaxy instance to the Galaxy table
 *
 * @param
 *          $form
 * @param
 *          $form_state
 */
function tripal_galaxy_admin_configuration_form_submit($form, &$form_state) {

  global $user;

  // Initialize the data to be sent to the tripal_galaxy table
  $data = array(
    'servername' => $form_state['values']['servername'],
    'description' => $form_state['values']['description'],
    'url' => $form_state['values']['url'],
    'username' => $form_state['values']['username'],
    'api_key' => $form_state['values']['api_key'],
    'uid' => $user->uid,
  );

  // If we are deleting the given instance, go through the appropriate
  // motions...
  if ($form_state['clicked_button']['#name'] == 'delete') {
    $galaxy_instance = db_query("DELETE FROM {tripal_galaxy} WHERE servername = :servername",
        array(':servername' => $form_state['values']['servername']));

    // Set error if appripriate
    if ($galaxy_instance === FALSE || $galaxy_instance == NULL) {
      drupal_set_message(t('Error deleting record'), 'error');
      return;
    }
    else {
      // If there is no error, redirect.
      drupal_set_message(t('Galaxy instance has been deleted'));
      drupal_goto("/admin/tripal/extension/galaxy");
    }
  }
  // If we are not deleting, are we submitting?
  else if ($form_state['clicked_button']['#name'] == 'submit') {
    // If there is a galaxy ID this will be an edit.
    if ($form_state['values']['galaxy_id']) {
      $data['galaxy_id'] = $form_state['values']['galaxy_id'];
      $galaxy_instance = drupal_write_record('tripal_galaxy', $data, 'galaxy_id');
    } // Otherwise it's an insert.
    else {
      $galaxy_instance = drupal_write_record('tripal_galaxy', $data);
    }

    // Insert the new Galaxy Instance into Drupal, return where appropriate.
    if (!$galaxy_instance) {
      drupal_set_message(t('Problem inserting galaxy instance'), 'error');
      return;
    }

    // Default message.
    drupal_set_message(t('Galaxy instance has been recorded' . var_dump($galaxy_instance)));
    drupal_goto("/admin/tripal/extension/galaxy");
  }

  // Else if we are testing the connection.
  else {
    $form_state['rebuild'] = TRUE;

    // Create a new galaxy instance using the obtained hostname and port
    // Then Authenticate
    $library = libraries_load('blend4php');
    $connect = tripal_split_galaxy_url($data['url']);
    $galaxy_instance = new GalaxyInstance($connect['host'], $connect['port'], $connect['use_https']);
    $galaxy_instance->setAPIKey($data['api_key']);

    // Tell the user whether the connection was successful based on
    // getVersion()
    if ($galaxy_instance->getVersion() == NULL) {
      drupal_set_message(t('No Connection'), 'error');
    }
    else {
      drupal_set_message(t('Connection successful'));
    }
  }
}