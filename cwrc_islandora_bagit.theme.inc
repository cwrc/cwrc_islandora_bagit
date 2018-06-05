<?php

/**
 * Returns HTML for a io_checkbox_tree form element.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties of the element.
 *
 * @return string
 *   The checkbox tree.
 *
 * @ingroup themeable
 */
function theme_io_checkbox_tree($variables) {
  $element = $variables['element'];
  $element['#children'] = drupal_render_children($element);

  $attributes = array();
  if (isset($element['#id'])) {
    $attributes['id'] = $element['#id'];
  }
  $attributes['class'][] = 'islandora-object-tree';
  $attributes['class'][] = 'clearfix';

  if (form_get_error($element)) {
    $attributes['class'][] = 'error';
  }

  if (!empty($element['#required'])) {
    $attributes['class'][] = 'required';
  }

  if (array_key_exists('#start_minimized', $element) && $element['#start_minimized']) {
    $attributes['class'][] = "islandora-object-tree-collapsed";
  }

  if (array_key_exists('#cascading_selection', $element) && $element['#cascading_selection']) {
    $attributes['class'][] = "islandora-object-tree-cascading-selection";
  }

  if (array_key_exists('#track_list', $element) && $element['#track_list']) {
    $attributes['class'][] = "islandora-object-tree-track-list-shown";
  }

  if (!empty($element['#attributes']['class'])) {
    $attributes['class'] = array_merge($attributes['class'], $element['#attributes']['class']);
  }

  $output = '<div' . drupal_attributes($attributes) . '>';
  $output .= (!empty($element['#children']) ? $element['#children'] : '');
  $output .= '</div>';
  return $output;
}

/**
 * Prints a list item with a checkbox and an unordered list of all the elements inside it.
 */
function theme_io_checkbox_tree_level($variables) {
  $element = $variables['element'];
  $sm = '';
  if (array_key_exists('#level_start_minimized', $element) && $element['#level_start_minimized']) {
    $sm = "style='display: none;'";
  }

  $output = "<ul class='islandora-object-tree-level clearfix' $sm>";
  $children = element_children($element);
  foreach ($children as $child) {
    $output .= '<li>';
    $output .= drupal_render($element[$child]);
    $output .= '</li>';
  }

  $output .= '</ul>';

  return $output;
}

/**
 * Prints a single item in the tree, followed by that item's children (which may be another checkbox_tree_level).
 */
function theme_io_checkbox_tree_item($variables) {
  $element = $variables['element'];
  $children = element_children($element);
  $output = '';

  $sm = $element['#level_start_minimized'] ? ' islandora-object-tree-collapsed' : '';

  if (is_array($children) && count($children) > 1) {
    $output .= "<div class='islandora-object-tree-button$sm'></div>";
  }
  elseif (!$element['#leaves_only']) {
    $output .= "<div class='no-islandora-object-tree-button'></div>";
  }

  foreach ($children as $child) {
    $output .= drupal_render($element[$child]);
  }

  return $output;
}

/**
 * This function prints a label that cannot be selected.
 */
function theme_io_checkbox_tree_label($variables) {
  $element = $variables['element'];
  $output = "<div class='parent-islandora-object'>" . $element['#value'] . "</div>";
  return $output;
}

/**
 * Process the checkbox_tree widget.
 *
 * This function processes the checkbox_tree widget.
 *
 * @param array $element
 *   The element to be drawn $element['#field_name'].
 * @param array $form_state
 *   The form state.
 *
 * @return array
 *   The processed element.
 */
function cwrc_islandora_bagit_process_checkbox_tree($element, $form_state) {
  if (is_array($form_state)) {
    if (!empty($element['#max_choices']) && $element['#max_choices'] != '-1') {
      drupal_add_js(array('cwrc_islandora_bagit' => array('trees' => array($element['#id'] => array('max_choices' => $element['#max_choices'])))), 'setting');
    }
    $allowed = array();

    $value = !empty($element['#default_value']) ? $element['#default_value'] : array();

    if (empty($element['#options'])) {
      $element['#options_tree'] = cwrc_islandora_bagit_get_object_hierarchy($element['#islandorar_object'], $allowed, '', '', $value, TRUE, array());
      $form_state['object_options_tree'] = $element['#options_tree'];
      if ($element['#max_choices'] == 1 && !$element['#required']) {
        array_unshift($element['#options_tree'], (object) array(
          'id' => '',
          'name' => 'N/A',
          'depth' => 0,
        ));
      }
      $element['#options'] = _cwrc_islandora_bagit_get_options($element['#options_tree']);
      $form_state['object_options'] = $element['#options'];
      $value = drupal_map_assoc(array_keys($element['#options']));
      $element['#default_value'] = $value;
    }

    $members = !empty($element['#options_tree']) ? $element['#options_tree'] : array();
    $max_choices = !empty($element['#max_choices']) ? $element['#max_choices'] : 1;
    if (array_key_exists('#select_parents', $element) && $element['#select_parents']) {
      $element['#attributes']['class'][] = 'select-parents';
      $element['#attributes']['class'][] = 'group-leader';
    }

    if ($max_choices != 1) {
      $element['#tree'] = TRUE;
    }

    $tree = new stdClass();
    $tree->children = $members;
    $element[] = _cwrc_islandora_bagit_build_level($element, $tree, $form_state, $value, $max_choices, array(), 1);
  }

  return $element;
}

/**
 * Builds a single member object in the tree widget.
 *
 * This function returns an element with a checkbox for a single member object.
 * If that member has children, it appends io_checkbox_tree_level element that
 * contains the children.  It is meant to be called recursively when the widget
 * is built.
 *
 * @param array $element
 *   The main checkbox_tree element.
 * @param $member
 *   A member object.  $member->children should be an array of the members
 *   objects that are that member's children.
 * @param array $form_state
 *   The form state.
 * @param $value
 *   The value of the element.
 * @param $max_choices
 *   The maximum number of allowed selections.
 *
 * @return
 *   A completed checkbox_tree_item element, which contains a checkbox and
 *   possibly a checkbox_tree_level element as well.
 */
function _cwrc_islandora_bagit_build_item($element, $member, $form_state, $value, $max_choices, $parent_ids, $parent, $depth) {
  $start_minimized = FALSE;
  $size = _cwrc_islandora_bagit_get_object_datastreams_human_readable_size($member);
  $object_name = array(
    '#type' => 'html_tag',
    '#tag' => 'span',
    '#value' => t('<span class="label">@label</span> (<span class="size">@size</span>)', array(
      '@label' => $member->label,
      '@size' => $size['human'],
    )),
    '#attributes' => array(
      'data-item-size' => $size['raw'],
      'class' => array('islandora-object-item-name'),
    ),
  );
  $object_name = drupal_render($object_name);
  $object_id = $member->id;
  if (array_key_exists('#start_minimized', $element)) {
    $start_minimized = $element['#start_minimized'];
  }

  $leaves_only = FALSE;
  if (array_key_exists('#leaves_only', $element)) {
    $leaves_only = $element['#leaves_only'];
  }

  $container = array(
    '#type' => 'io_checkbox_tree_item',
    '#max_choices' => $max_choices,
    '#leaves_only' => $leaves_only,
    '#object_name' => $object_name,
    '#level_start_minimized' => FALSE,
    '#depth' => $depth,
  );

  if (!$element['#leaves_only'] || count($member->children) == 0) {
    $e = array(
      '#type' => ($max_choices == 1) ? 'radio' : 'checkbox',
      '#title' => $object_name,
      '#on_value' => $object_id,
      '#off_value' => 0,
      '#return_value' => $object_id,
      '#parent_values' => $parent_ids,
      '#default_value' => isset($value[$object_id]) ? $object_id : NULL,
      '#attributes' => isset($element['#attributes']) ? $element['#attributes'] : array(),
      '#ajax' => isset($element['#ajax']) ? $element['#ajax'] : NULL,
    );
  }
  else {
    $e = array(
      '#type' => 'io_checkbox_tree_label',
      '#value' => $object_name,
    );
  }

  $container[$object_id] = $e;

  if (($depth + 1 <= $element['#max_depth'] || !$element['#max_depth']) && property_exists($member, 'children') && count($member->children) > 0) {
    $parents = $parent_ids;
    $parents[] = $object_id;
    $container[$object_id . '-children'] = _cwrc_islandora_bagit_build_level($element, $member, $form_state, $value, $max_choices, $parents, $depth + 1);
    $container['#level_start_minimized'] = $container[$object_id . '-children']['#level_start_minimized'];
  }
  else {
    $container[$object_id]['#attributes']['class'][] = 'group-leaf';
    if ($group_leader_key = array_search('group-leader', $container[$object_id]['#attributes']['class'])) {
      unset($container[$object_id]['#attributes']['class'][$group_leader_key]);
    }
  }

  return $container;
}

/**
 * Builds a level in the tree widget.
 *
 * This function returns an element that has a number of checkbox_tree_item elements
 * as children. It is meant to be called recursively when the widget is built.
 *
 * @param array $element
 *   The main checkbox_tree element.
 * @param $member
 *   A member object. $member->children should be an array of the member
 *   objects that are that term's children.
 * @param $form_state
 *   The form state.
 * @param $value
 *   The value of the element.
 * @param int $max_choices
 *   The maximum number of allowed selections.
 *
 * @return array
 *   A completed checkbox_tree_level element.
 */
function _cwrc_islandora_bagit_build_level($element, $member, $form_state, $value, $max_choices, $parent_ids, $depth) {
  $start_minimized = FALSE;
  if (array_key_exists('#start_minimized', $element)) {
    $start_minimized = $element['#start_minimized'];
  }

  $leaves_only = FALSE;
  if (array_key_exists('#leaves_only', $element)) {
    $leaves_only = $element['#leaves_only'];
  }

  $container = array(
    '#type' => 'io_checkbox_tree_level',
    '#max_choices' => $max_choices,
    '#leaves_only' => $leaves_only,
    '#start_minimized' => $start_minimized,
    '#depth' => $depth,
  );

  $container['#level_start_minimized'] = $depth > 1 && $element['#start_minimized'] && !($member->children_selected);

  foreach ($member->children as $child) {
    $container[$child->id] = _cwrc_islandora_bagit_build_item($element, $child, $form_state, $value, $max_choices, $parent_ids, $container, $depth);
  }

  return $container;
}

/**
 * Validates the objects checkbox tree.
 *
 * This function sets the value of the tree widgets into a form that Drupal
 * can understand, and also checks if the field is required and has been
 * left empty.
 *
 * @param array $element
 *   The element to be validated.
 * @param array $form_state
 *   The state of the form.
 *
 * @return array
 *   The validated element.
 */
function _cwrc_islandora_bagit_io_checkbox_tree_validate(&$element, &$form_state) {
  $items = _cwrc_islandora_bagit_tree_flatten($element, $form_state);
  $value = array();

  if ($element['#max_choices'] !== 1) {
    foreach ($items as $child) {
      if (!empty($child['#value'])) {
        $value[] = array($element['#value_key'] => $child['#value']);

        // If the element is leaves only and select parents is on, then
        // automatically add all the parents of each selected value.
        if ($element['#select_parents'] && $element['#leaves_only']) {
          foreach ($child['#parent_values'] as $parent_id) {
            if (!in_array(array($element['#value_key'] => $parent_id), $value)) {
              $value[] = array($element['#value_key'] => $parent_id);
            }
          }
        }
      }
    }
  }
  else {
    // If it's a tree of radio buttons, they all have the same value, so we can
    // just grab the value of the first one.
    if (count($items) > 0) {
      $child = reset($items);
      if (!empty($child['#value'])) {
        $value[] = array($element['#value_key'] => $child['#value']);
      }
    }
  }

  if (!$value && $element['#required']) {
    // The title is already check_plained so it's appropriate to use !.
    form_error($element, t('!name field is required.', array('!name' => $element['#title'])));
  }

  form_set_value($element, $value, $form_state);
  return $element;
}
