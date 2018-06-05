(function($) {

  /**
   * Attaches the tree behavior to the term widget form.
   */
  Drupal.behaviors.cwrcIslandoraBagItTree = {
    attach: function(context, settings) {
      var module_settings = settings.cwrc_islandora_bagit;
      var message_placeholder = $('.feedback-placeholder');
      // Bind the term expand/contract button to slide toggle the list underneath.
      $('.islandora-object-tree-button', context).once('islandora-object-tree-button').click(function() {
        $(this).toggleClass('islandora-object-tree-collapsed');
        $(this).siblings('ul').slideToggle('fast');
      });

      $('.islandora-object-tree', context).once('islandora-object-tree', function() {
        // On page load, check whether the maximum number of choices is already selected.
        // If so, disable the other options.
        var tree = $(this);
        checkMaxChoices(tree, false);
        $(this).find('input[type=checkbox]').change(function() {
          checkMaxChoices(tree, $(this));
        });
      });

      // Store the checkboxes to manage scope
      var $allCheckboxes = $('.islandora-object-tree-cascading-selection ul input[type=checkbox]', context);
      $allCheckboxes.on('change', function(event) {
        // Store what changed and what it changed to
        var $this = $(this),
          value = $this.prop('checked'),
          // Because val returns true or false, we have to store
          // something that the input tag understands
          checkedValue = value ? 'checked' : '',
          // Placeholder for parent and children elements
          $parent, $children;

        if ($this.hasClass('group-leader')) {
          // Only works with one level of nesting at this time
          $children = $this.parent().siblings('ul').find('input[type=checkbox]');
          $children.each( function(idx, item) {
            $(item).prop( 'checked', checkedValue );
          });
        }
        else {
          // The parent checkbox is the closest nested list's group-leader
          $parent = $this.closest('ul')
            .siblings('.form-type-checkbox')
            .children('.group-leader');

          // Nasty DOM sniffing here... could probably be cleaned up
          $children = $this.closest('ul').find('> li input[type=checkbox]').add($this);

          // Check if the children are all checked
          var scopedNumChecked = 0,
            numItems = $children.length,
            areAllItemsChecked;

          // Count the number of items that are checked
          $children.each(function( idx, item ) {
            if($(item).prop('checked')) {
              scopedNumChecked++;
            }
          });

          areAllItemsChecked = scopedNumChecked === numItems;

          // If all the items are checked, then check off the parent
          if(areAllItemsChecked) {
            $parent.prop('checked', 'checked');
            $parent.prop('disabled', '');
            // If no items are checked, then uncheck the parent and make sure
            // it's still clickable
          }
          else if ( scopedNumChecked === 0 ){
            $parent.prop('disabled', '');
            $parent.prop('checked', '');
            // If some are checked, then the parent is disabled
          }
          else {
            $parent.prop('checked', 'checked');
            $parent.prop('disabled', 'disabled');
          }
        }
        // Recalculating and updating the tree size on change
        calculateTreeSize(context, module_settings, message_placeholder);
      });

      // Set the main label size on page load.
      calculateTreeSize(context, module_settings, message_placeholder);
    }
  };

  function calculateTreeSize(context, settings, message_placeholder) {
    var treeSize = 0,
      treeSizeWithUnit,
      warning = $('#cwrc-islandora-bagit-max-size-exceeded'),
      noWarning = warning.length === 0;

    // Set the main label size on page load.
    $('.islandora-object-item-name', context).each(function (i) {
      var form_checkbox = $(this).parent().siblings('.form-checkbox');
      if (form_checkbox.prop('checked')) {
        treeSize += $(this).data('item-size');
      }
    });

    // Updating hidden field value.
    $('.objects-tree-size', context).val(treeSize);

    var reachedMaxSize = treeSize > settings.max_size;
    if (reachedMaxSize && noWarning) {
      $(settings.max_size_warning).appendTo(message_placeholder).slideDown("slow");
    }

    if (!reachedMaxSize) {
      warning.slideUp("slow").remove();
    }

    $('#edit-submit', context).prop('disabled', reachedMaxSize);
    treeSizeWithUnit = '(' + formatBytes(treeSize) + ')';
    var treeSizeDomNode = $('.form-type-io-checkbox-tree > label .tree-size');
    treeSizeDomNode.text(treeSizeWithUnit);
  }

  /**
   * Add a new item to the track list.
   * If more than one item can be selected, the new item is positioned to
   * match the order of the terms in the checkbox tree.
   *
   * @param track_list_container Container where the new item will be added.
   *
   * @param item_text Text of the item to add.
   *
   * @param control_id Id of the checkbox/radio control the item matches.
   *
   * @param control_type Control type - 'checkbox' or 'radio'.
   */
  function addItemToTrackList(track_list_container, item_text, control_id, control_type) {
    var new_item = $('<li class="track-item">' + item_text + '</li>');
    new_item.data('control_id', control_id);

    // Add an id for easy finding of the item.
    new_item.attr('id', control_id + '_list');

    // Process radio controls - only one item can be selected.
    if ( control_type == 'radio') {
      // Find the existing element on the track list, if there is one.
      var current_items = track_list_container.find('li');

      // If there are no items on the track list, add the new item.
      if ( current_items.size() == 0 ) {
        track_list_container.append(new_item);
      }
      else {
        // There is an item on the list.
        var current_item = $(current_items.get(0));

        // Is the item we want to add different from what is there?
        if ( current_item.data('control_id') != control_id ) {
          // Remove exiting element from track list, and add the new one.
          current_item.remove();
          track_list_container.append(new_item);
        }
      }
      return;
    }

    // Using checkboxes, so there can be more than one selected item.
    // Find the right place to put the new item, to match the order of the
    // checkboxes.
    var list_items = track_list_container.find('li');
    var item_comparing_to;

    // Flag to tell whether the item was inserted.
    var inserted_flag = false;
    list_items.each(function(index){
      item_comparing_to = $(list_items[index]);

      // If item is already on the track list, do nothing.
      if ( control_id == item_comparing_to.data('control_id') ) {
        inserted_flag = true;
        // Returning false stops the loop.
        return false;
      }
      else if ( control_id < item_comparing_to.data('control_id') ) {
        // Add it here.
        item_comparing_to.before(new_item);
        inserted_flag = true;
        // Returning false stops the loop.
        return false;
      }
    });

    // If not inserted yet, add new item at the end of the track list.
    if ( ! inserted_flag ) {
      track_list_container.append(new_item);
    }
  }

  /**
   * Show the 'nothing selected' message if it applies.
   *
   * @param track_list_container Where the message is to be shown.
   */
  function showNothingSelectedMessage(track_list_container) {
    //Is the message there already?
    var message_showing =
      (track_list_container.find('.term_ref_tree_nothing_message').size() != 0);

    //Number of real items showing.
    var num_real_items_showing =
      message_showing
        ? track_list_container.find('li').size() - 1
        : track_list_container.find('li').size();
    if ( num_real_items_showing == 0 ) {
      //No items showing, so show the message.
      if ( ! message_showing ) {
        track_list_container.append(
          '<li class="term_ref_tree_nothing_message">' + termReferenceTreeNothingSelectedText + '</li>'
        );
      }
    }
    else { // !(num_real_items_showing == 0)
      //There are real items.
      if ( message_showing ) {
        track_list_container.find('.term_ref_tree_nothing_message').remove();
      }
    }
  }

  /**
   * Remove the 'nothing selected' message. Makes processing easier.
   *
   * @param track_list_container Where the message is shown.
   */
  function removeNothingSelectedMessage(track_list_container) {
    track_list_container.find('.term_ref_tree_nothing_message').remove();
  }

  /**
   * This helper function checks if the maximum number of choices is already
   * selected. If so, it disables all the other options.  If not, it enables
   * them.
   */
  function checkMaxChoices(item, checkbox) {
    var maxChoices = -1;
    try {
      maxChoices = parseInt(Drupal.settings.cwrc_islandora_bagit.trees[item.attr('id')]['max_choices']);
    }
    catch (e){}
    var count = item.find(':checked').length;

    if (maxChoices > 0 && count >= maxChoices) {
      item.find('input[type=checkbox]:not(:checked)').attr('disabled', 'disabled').parent().addClass('disabled');
    }
    else {
      item.find('input[type=checkbox]').removeAttr('disabled').parent().removeClass('disabled');
    }

    if(checkbox) {
      if(item.hasClass('select-parents')) {
        var track_list_container = item.find('.islandora-object-tree-track-list');
        var input_type =
          ( item.has('input[type=checkbox]').size() > 0 ) ? 'checkbox' : 'radio';

        if(checkbox.attr('checked')) {
          checkbox.parents('ul.islandora-object-tree-level li').children('div.form-item').children('input[type=checkbox]').each(function() {
            $(this).attr('checked', checkbox.attr('checked'));

            if(track_list_container) {
              label_element = $(this).next();
              addItemToTrackList(
                track_list_container,         //Where to add new item.
                label_element.html(),         //Text of new item.
                $(label_element).attr('for'), //Id of control new item is for.
                input_type                    //checkbox or radio
              );
            }
          });
        }
      }
    }
  }

  function formatBytes(bytes, decimals) {
    if(bytes == 0) return '0 Bytes';
    var k = 1024,
      dm = decimals || 2,
      sizes = ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PB', 'EB', 'ZB', 'YB'],
      i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
  }

})(jQuery);
