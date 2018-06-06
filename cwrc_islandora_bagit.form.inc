<?php

/**
 * @file
 * Bagit form.
 */

/**
 * Bagit form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 * @param \IslandoraFedoraObject $islandora_object
 *   The islandora object.
 *
 * @return array
 *   The form render array.
 */
function cwrc_islandora_bagit_create_bag_form($form, &$form_state, $islandora_object) {
  $module_name = 'cwrc_islandora_bagit';
  $path = drupal_get_path('module', $module_name);
  $lazy_load_trigger_class = 'lazy-loader-trigger';

  $values = !empty($form_state['values']) ? $form_state['values'] : array();
  $bagit_values = !empty($form_state['bagit_values']) ? $form_state['bagit_values'] : array();
  $options_tree = !empty($form_state['object_options_tree']) ? $form_state['object_options_tree'] : array();
  $options = $options_tree ? $form_state['object_options'] : array();

  if ($bagit_values) {
    $lazy_load_triggered = (bool) $bagit_values['lazy_load_trigger'];
  }
  else {
    $lazy_load_triggered = !empty($values['lazy_load_trigger']) ? (bool) $values['lazy_load_trigger'] : FALSE;
  }

  // Preparing settings to pass to our js.
  $js_settings[$module_name] = array(
    'lazy_load_trigger_class' => $lazy_load_trigger_class,
    'trigger_lazy_load' => !($lazy_load_triggered && $bagit_values),
    'max_size' => CWRC_ISLANDORA_BAGIT_MAX_COLLECTION_SIZE,
    'max_size_warning' => _cwrc_islandora_bagit_max_collection_size_message(),
  );

  $form['lazy_load_trigger'] = array(
    '#type' => 'checkbox',
    '#title' => t('Custom checkbox to trigger objects tree lazy loading.'),
    '#title_display' => 'invisible',
    '#attributes' => array('class' => array($lazy_load_trigger_class)),
    '#ajax' => array(
      'callback' => 'cwrc_islandora_bagit_create_bag_form_form_ajax_callback',
      'wrapper' => 'objects-tree-wrapper',
      'effect' => 'fade',
      'progress' => array(
        'type' => 'throbber',
        'message' => t('Loading...<br>Depending on your selection, this may take a few minutes.'),
      ),
    ),
    '#attached' => array(
      'js' => array(
        array('type' => 'setting', 'data' => $js_settings),
        array(
          'type' => 'file',
          'data' => $path . '/js/tree_lazy_loading.js',
        ),
      ),
      'css' => array(
        $path . '/css/tree_lazy_loading.css',
        'sites/default/libraries/CWRC-Writer/src/js/lib/cwrcDialogs/fonts/font-awesome/css/font-awesome.min.css',
      ),
    ),
    '#default_value' => $lazy_load_triggered ? 1 : 0,
  );

  $form['objects_tree_wrapper'] = array(
    '#prefix' => '<div id="objects-tree-wrapper">',
    '#suffix' => '</div>',
    '#type' => 'container',
  );

  if ($lazy_load_triggered || $bagit_values) {
    if (!$bagit_values) {
      $default_values = !empty($values['objects_tree']) ? $values['objects_tree'] : array();
      $tree_size = !empty($values['objects_tree_size']) ? $values['objects_tree_size'] : 0;
    }
    else {
      $default_values = $bagit_values['objects_tree'];
      $tree_size = $bagit_values['objects_tree_size'];
    }

    $form['objects_tree_wrapper']['#prefix'] = t('<p>You have chosen to download <a href="/islandora/object/@id">@label</a>, containing:</p>', array(
      '@id' => $islandora_object->id,
      '@label' => $islandora_object->label,
    ));
    if (
      _cwrc_islandora_bagit_object_is_collection_policy($islandora_object) ||
      _cwrc_islandora_bagit_object_is_paged_content($islandora_object)
    ) {
      $form['objects_tree_wrapper']['objects_tree'] = array(
        '#islandorar_object' => $islandora_object,
        '#title' => t('@label <span class="tree-size"></span>', array(
          '@label' => $islandora_object->label,
        )),
        '#type' => 'io_checkbox_tree',
        '#default_value' => $default_values,
        '#max_choices' => -1,
        '#max_depth' => '',
        '#start_minimized' => 0,
        '#leaves_only' => 0,
        '#filter_view' => '',
        '#select_parents' => 1,
        '#cascading_selection' => 1,
        '#value_key' => 'object_id',
        '#required' => TRUE,
        '#attached' => array(
          'js' => array($path . '/js/tree.js'),
          'css' => array($path . '/css/tree.css'),
        ),
        '#options' => $options,
        '#options_tree' => $options_tree,
        '#element_validate' => array('_cwrc_islandora_bagit_io_checkbox_tree_validate'),
        '#value' => array(),
      );
    }

    $form['objects_tree_wrapper']['objects_tree_size'] = array(
      '#type' => 'hidden',
      '#attributes' => array(
        'class' => ['objects-tree-size'],
      ),
      '#value' => $tree_size,
    );
  }

  $form['actions'] = array(
    '#type' => 'actions',
    '#prefix' => '<div class="feedback-placeholder"></div>',
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => "/islandora/object/{$islandora_object->id}",
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Download'),
  );
  return $form;
}

/**
 * Ajax callback for object tree rendering.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 *
 * @return array
 *   The objects tree wrapper array.
 */
function cwrc_islandora_bagit_create_bag_form_form_ajax_callback($form, $form_state) {
  return $form['objects_tree_wrapper'];
}

/**
 * Bagit form validate callback.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function cwrc_islandora_bagit_create_bag_form_validate($form, &$form_state) {
  $tree_size = $form_state['values']['objects_tree_size'];
  if ($tree_size > CWRC_ISLANDORA_BAGIT_MAX_COLLECTION_SIZE) {
    form_set_error(array(), _cwrc_islandora_bagit_max_collection_size_message());
  }
  elseif ($tree_size == 0) {
    dpm($form_state['values']);
    form_set_error(array(), t('The tree is empty or you have un selected or the objects.'));
  }
}

/**
 * Bagit form submit callback.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function cwrc_islandora_bagit_create_bag_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  // Saving in bagit values so that they can be reuse on the confirmation step.
  $form_state['bagit_values'] = $values;

  // Process the batch download.
  $tree_hierarchy = $form['objects_tree_wrapper']['objects_tree']['#options_tree'];
  $selected_objects = $form_state['bagit_values']['objects_tree'];
  $ids = array();
  foreach ($selected_objects as $selected) {
    $id = $selected['object_id'];
    $ids[$id] = $id;
  }
  $objects_to_include = _cwrc_islandora_bagit_get_objects_to_bag($tree_hierarchy, $ids);
  $main_collection = $form['objects_tree_wrapper']['objects_tree']['#islandorar_object'];
  cwrc_islandora_bagit_create_bag_prepare_batch($objects_to_include, $main_collection);
}

/**
 * Gets objects to bag.
 *
 * @param \IslandoraFedoraObject[] $tree_hierarchy_objects
 *   The current tree hierarchy.
 * @param array $object_ids
 *   The selected object ids.
 *
 * @return array
 *   Of objects to bag.
 */
function _cwrc_islandora_bagit_get_objects_to_bag($tree_hierarchy_objects, $object_ids) {
  $objects_to_bag = array();

  foreach ($tree_hierarchy_objects as $fedoraObject) {
    $id = $fedoraObject->id;
    if (isset($object_ids[$id])) {
      $objects_to_bag[$id] = $fedoraObject;
    }
    $children_to_bag = _cwrc_islandora_bagit_get_objects_to_bag($fedoraObject->children, $object_ids);
    $objects_to_bag = array_merge($objects_to_bag, $children_to_bag);
  }

  return $objects_to_bag;
}

function cwrc_islandora_bagit_create_bag_prepare_batch($objects_to_include, $collection) {
  if (variable_get('islandora_bagit_multiple_bag_type', 'object') === 'object') {
    cwrc_islandora_bagit_create_object_bag_batch($objects_to_include, $collection);
  }

  if (variable_get('islandora_bagit_multiple_bag_type', 'object') === 'collection') {
    cwrc_islandora_bagit_create_collection_bag_batch($objects_to_include, $collection);
  }
}

function _cwrc_islandora_bagit_max_collection_size_message() {
  return t('<div id="cwrc-islandora-bagit-max-size-exceeded" class="clearfix" style="display: none;"><i class="fa fa-exclamation-triangle" aria-hidden="true"></i><p>
Your current selection exceeds the 2 GB maximum size of collection bags. Reduce 
the size of your bag by un-checking some of the descendants.<br/>You may create 
one ore more additional bags for the items you have unchecked here.
  </p></div>');
}
