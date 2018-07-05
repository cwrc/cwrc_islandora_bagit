(function ($, Drupal) {

  'use strict';

  Drupal.behaviors.cwrcIslandoraBagItTreeLazyLoad = {
    attach: function (context, settings) {
      var module_settings = settings.cwrc_islandora_bagit;
      var submit = $('#edit-submit', context);
      var $lazy_loader = $('.' + module_settings.lazy_load_trigger_class, context);
      if (module_settings.trigger_lazy_load && !$lazy_loader.hasClass('lazy-load-trigger-processed')) {
        // Triggering the lazy loading of objects tree.
        $lazy_loader.hide()
          .prop('checked', true)
          .addClass('lazy-load-trigger-processed')
          .trigger("change");
        // Disabling the submit button while the js is running.
        submit.hide().prop('disabled', true);
      }

      // When ajax is complete tasks.
      $(document).ajaxComplete(function(e, r, s) {
        var element_name = s.extraData._triggering_element_name,
          is_lazy_loader = element_name === 'lazy_load_trigger',
          lazy_loader_triggered =  is_lazy_loader && s.extraData.lazy_load_trigger === '1',
          success_request = r.status === 200,
          noWarning = $('#cwrc-islandora-bagit-max-size-exceeded').length === 0,
          treeSize = $('.objects-tree-size', context).val(),
          disableSubmit = !(lazy_loader_triggered && success_request && noWarning) || treeSize == 0;

        // Enabling the submit button back.
        submit.show().prop('disabled', disableSubmit);
      });

      function onFormSubmit(e) {
        var $form = $(e.currentTarget);
        var formValues = $form.serialize();
        var previousValues = $form.attr('data-drupal-form-submit-last');
        if (previousValues === formValues) {
          e.preventDefault();
        }
        else {
          $form.attr('data-drupal-form-submit-last', formValues);
        }
      }
      // Preventing double click submit.
      $('body').once('form-single-submit').on('submit.singleSubmit', 'form:not([method~="GET"])', onFormSubmit);
    }
  };
})(jQuery, Drupal);
