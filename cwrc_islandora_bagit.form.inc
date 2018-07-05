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
  // For object which are not a collection policy let the normal bagit deal
  // with them.
  if (!_cwrc_islandora_bagit_object_is_collection_policy($islandora_object)) {
    $link = islandora_bagit_create_bag($islandora_object);
    if (strpos($link, 'Download the Bag') !== FALSE) {
      $form['link'] = array('#markup' => $link);
      return $form;
    }
  }
  else {
    $form_state['root_tree_islandora_object'] = $islandora_object;
  }

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
    $triggering_element = $form_state['triggering_element'];
    if (!$bagit_values) {
      $default_values = !empty($values['objects_tree']) ? $values['objects_tree'] : array();
      $tree_size = !empty($values['objects_tree_size']) ? $values['objects_tree_size'] : 0;
    }
    else {
      $default_values = $bagit_values['objects_tree'];
      $tree_size = $bagit_values['objects_tree_size'];
    }
    $islandora_object_title = t('@label ( <span class="select-all-none-wrapper"></span> )', array(
      '@label' => $islandora_object->label,
    ));

    $is_collection_policy = _cwrc_islandora_bagit_object_is_collection_policy($islandora_object);
    $is_paged_content = !$is_collection_policy ? _cwrc_islandora_bagit_object_is_paged_content($islandora_object) : FALSE;
    if ($is_collection_policy || $is_paged_content) {
      if ((!$options || !$options_tree) && $triggering_element['#type'] !== 'submit') {
        $allowed = array();
        $options_tree = cwrc_islandora_bagit_get_object_hierarchy(
          $islandora_object,
          $allowed,
          '',
          '',
          $default_values,
          TRUE,
          array()
        );
        $options = _cwrc_islandora_bagit_get_options($options_tree);
        $default_values = drupal_map_assoc(array_keys($options));
        $form_state['object_options_tree'] = $options_tree;
        $form_state['object_options'] = $options;
        $form_state['root_tree_islandora_object'] = $islandora_object;
      }
      elseif ($triggering_element['#type'] === 'submit' && isset($triggering_element['#tree_parent'])) {
        $trigger_object = $triggering_element['#tree_parent'];
        $options_tree = _cwrc_islandora_bagit_create_bag_form_ajax_load_more_options_tree(
          $trigger_object,
          $form_state
        );
        $options = _cwrc_islandora_bagit_get_options($options_tree);
        $form_state['object_options_tree'] = $options_tree;
        $form_state['object_options'] = $options;
      }
      $form['objects_tree_wrapper']['objects_tree'] = array(
        '#islandorar_object' => $islandora_object,
        '#prefix' => _cwrc_islandora_bagit_get_object_tree_element_prefix($islandora_object),
        '#suffix' => '</div>',
        '#title' => $islandora_object_title,
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
        '#weight' => 1,
        '#tree_load_more_callback' => 'cwrc_islandora_bagit_create_bag_form_ajax_load_more_callback',
        '#tree_load_more_submit' => array('cwrc_islandora_bagit_create_bag_form_ajax_load_more'),
        '#tree_load_more_wrapper' => 'cwrc-islandora-bagit-tree-checkboxes',
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
 * Ajax load more callback.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 *
 * @return array
 *   the element to be rebuilt on ajax callback.
 */
function cwrc_islandora_bagit_create_bag_form_ajax_load_more_callback($form, $form_state) {
  return $form['objects_tree_wrapper']['objects_tree'];
}

/**
 * Ajax load more submit.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function cwrc_islandora_bagit_create_bag_form_ajax_load_more($form, &$form_state) {
  $triggering_element = $form_state['triggering_element'];
  $parent_object = &$triggering_element['#tree_parent'];
  if (empty($form_state['object_options_tree_loaded_more_count'][$parent_object->id])) {
    $form_state['object_options_tree_loaded_more_count'][$parent_object->id] = 1;
  }
  else {
    $form_state['object_options_tree_loaded_more_count'][$parent_object->id]++;
  }

  $form_state['rebuild'] = TRUE;
}

function _cwrc_islandora_bagit_create_bag_form_ajax_load_more_options_tree($trigger_object, &$form_state) {
  $options_tree = !empty($form_state['object_options_tree']) ? $form_state['object_options_tree'] : array();
  $trigger_object->COLLECTION_MEMBERS_OFFSET = $form_state['object_options_tree_loaded_more_count'][$trigger_object->id];
  $allowed = array();
  $default = array();
  $processed = array();
  $filter = '';
  $label = '';
  $trigger_object->children = cwrc_islandora_bagit_get_object_hierarchy(
    $trigger_object,
    $allowed,
    $filter,
    $label,
    $default,
    FALSE,
    $processed,
    0,
    $form_state['object_options_tree_loaded_more_count'][$trigger_object->id]
  );

  // Updating the main/root tree object if necessary.
  $main_object = $form_state['root_tree_islandora_object'];
  if ($trigger_object->id === $main_object->id) {
    $form_state['root_tree_islandora_object'] = $trigger_object;
    $options_tree = $trigger_object->children;
  }
  else {
    // Updating the tree which was loaded more.
    $options_tree = _cwrc_islandora_bagit_create_bag_form_ajax_load_more_options_tree_update_trigger_object($options_tree, $trigger_object);
    $form_state['root_tree_islandora_object']->children = $options_tree;
  }

  return $options_tree;
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
  $triggering_element = $form_state['triggering_element'];
  if (!empty($triggering_element['#tree_parent']) || $triggering_element['#type'] === 'checkbox') {
    return;
  }
  $tree_size = isset($form_state['values']['objects_tree_size']) ? $form_state['values']['objects_tree_size'] : 0;
  if ($tree_size > CWRC_ISLANDORA_BAGIT_MAX_COLLECTION_SIZE) {
    form_set_error(array(), _cwrc_islandora_bagit_max_collection_size_message());
  }
  elseif ($tree_size == 0 && empty($form_state['values']['objects_tree'])) {
    form_set_error(array(), t('The tree is empty or you have un selected all the objects.'));
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
  $triggering_element = $form_state['triggering_element'];
  if (!empty($triggering_element['#tree_parent']) || empty($form_state['bagit_values']['objects_tree'])) {
    return;
  }

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
  return t('<div id="cwrc-islandora-bagit-max-size-exceeded" class="clearfix" style="display: none;"><i class="fa fa-exclamation-triangle error" aria-hidden="true"></i><p>
Your current selection exceeds the 2 GB maximum size of collection bags. Reduce 
the size of your bag by un-checking some of the descendants.<br/>You may create 
one ore more additional bags for the items you have unchecked here.
  </p></div>');
}

function _cwrc_islandora_bagit_create_bag_form_ajax_load_more_options_tree_update_trigger_object($options_tree, $trigger_object) {
  foreach ($options_tree as $object) {
    if ($object->id === $trigger_object->id) {
      $options_tree[$object->id] = $trigger_object;
    }
    elseif ($object->children) {
      $object->children = _cwrc_islandora_bagit_create_bag_form_ajax_load_more_options_tree_update_trigger_object($object->children, $trigger_object);
    }
  }
  return $options_tree;
}
