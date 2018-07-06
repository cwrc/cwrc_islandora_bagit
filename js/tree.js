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
        check_max_choices(tree, false);
        $(this).find('input[type=checkbox]').change(function() {
          check_max_choices(tree, $(this));
        });
      });

      // Store the checkboxes to manage scope
      var all_checkboxes = $('.islandora-object-tree-cascading-selection ul input[type=checkbox]', context);
      all_checkboxes.on('change', function(event) {
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
            $(item).prop('checked', checkedValue);
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
        calculate_tree_size(context, module_settings, message_placeholder);
      });
    }
  };

  Drupal.behaviors.cwrcIslandoraBagItTreeSelectAllNone = {
    attach: function(context, settings) {
      var module_settings = settings.cwrc_islandora_bagit;
      var message_placeholder = $('.feedback-placeholder');

      $(document).ajaxComplete(function(e, r, s) {
        var element_name = s.extraData._triggering_element_name,
          is_lazy_loader = element_name === 'lazy_load_trigger',
          lazy_loader_triggered =  is_lazy_loader && s.extraData.lazy_load_trigger === '1',
          success_request = r.status === 200,
          noWarning = $('#cwrc-islandora-bagit-max-size-exceeded').length === 0,
          treeSize = $('.objects-tree-size', context).val(),
          enableSelAllNone = (lazy_loader_triggered && success_request && noWarning) || treeSize != 0;

        /*
         * Add Select all/none links to specified checkboxes
         */
        var selected = $('.islandora-object-tree.bagit-tree-select-all-none:not(.bagit-tree-processed)');
        if (selected.length && enableSelAllNone) {
          var selAll = Drupal.t('Select All');
          var selNone = Drupal.t('Select None');

          // Set up a prototype link and event handlers.
          var link = $('<a class="bagit-tree-toggle" href="#">'+ selAll +'</a>');
          link.click(function(event) {
            // Don't actually follow the link...
            event.preventDefault();
            event.stopPropagation();

            var noneSelected = selAll === $(this).text();
            $(this).html(noneSelected ? selNone : selAll);
            $('.bagit-tree-form-checkbox').each(function(index, item) {
              $(item).prop('checked', noneSelected);
            });

            // Recalculating and updating the tree size on change.
            calculate_tree_size(context, module_settings, message_placeholder);
          });

          // Add link to the page for each set of checkboxes.
          selected
            .addClass(function() {
              // Clone the link prototype and insert into the DOM.
              var newLink = link.clone(true);

              newLink.insertBefore($('.select-all-none-wrapper', '.form-type-io-checkbox-tree > label'));

              // If all checkboxes are already checked by default then switch to Select None.
              if ($('input:checkbox:checked', this).length === $('input:checkbox', this).length) {
                newLink.click();
              }
              return 'bagit-tree-processed';
            });
        }
      });
    }
  };

  /*
   * Helper functions
   */

  function calculate_tree_size(context, settings, message_placeholder) {
    var treeSize = 0,
      treeSizeWithUnit,
      warning = $('#cwrc-islandora-bagit-max-size-exceeded'),
      noWarning = warning.length === 0;

    // Set the main label size on page load.
    $('.islandora-object-item-name').each(function (i) {
      var form_checkbox = $(this).parent().siblings('.form-checkbox');
      if (form_checkbox.prop('checked')) {
        treeSize += $(this).data('item-size');
      }
    });

    // Updating hidden field value.
    $('.objects-tree-size').val(treeSize);

    var reachedMaxSize = treeSize > settings.max_size;
    if (reachedMaxSize && noWarning) {
      $(settings.max_size_warning).appendTo(message_placeholder).slideDown("slow");
    }

    if (!reachedMaxSize) {
      warning.slideUp("slow").remove();
    }

    $('#edit-submit').prop('disabled', reachedMaxSize);
    treeSizeWithUnit = format_bytes(treeSize);
    var treeSizeDomNode = $('.form-intro span.tree-size');
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
  function add_item_to_track_list(track_list_container, item_text, control_id, control_type) {
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
   * This helper function checks if the maximum number of choices is already
   * selected. If so, it disables all the other options.  If not, it enables
   * them.
   */
  function check_max_choices(item, checkbox) {
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
          checkbox.parents('ul.islandora-object-tree-level li')
            .children('div.form-item')
            .children('input[type=checkbox]').each(function() {
              $(this).attr('checked', checkbox.attr('checked'));

              if (track_list_container) {
                label_element = $(this).next();
                add_item_to_track_list(
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

  /**
   * Format the
   * @param bytes
   * @param decimals
   * @returns {string}
   */
  function format_bytes(bytes, decimals) {
    if(bytes == 0) return '0 Bytes';
    var k = 1024,
      dm = decimals || 2,
      sizes = ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PB', 'EB', 'ZB', 'YB'],
      i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
  }

})(jQuery);
