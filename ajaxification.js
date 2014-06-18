(function($) {

  // Initialize settings.
  var settings = Drupal.settings.ajaxification;

  // Initialize code.
  var ajaxification = initAjaxification();

  // Go through configurations, and choose one.
  $.each(settings.configurations, function(cfgId, cfg) {
    var selector = (cfg.replaceWrapperContextSelector ? cfg.replaceWrapperContextSelector + ' ' : '')
        + cfg.replaceWrapperSelector;
    ajaxification.$replaceWrapper = $(selector);
    if (ajaxification.$replaceWrapper.size()) {
      ajaxification.cfgId = cfgId;
      ajaxification.cfg = cfg;
      return false;
    }
  });
  if (!ajaxification.$replaceWrapper.size()) {
    return;
  }
  if (ajaxification.cfg.historyApiOnly && !window.history.pushState) {
    return;
  }

  ajaxification.handleHistoryButtons();

  // Save initial page content before Drupal behaviors applied.
  ajaxification.savePageState(window.location.href);

  /**
   * Do ajaxification.
   */
  Drupal.behaviors.ajaxification = {
    attach : function(context, _) {

      var $ajaxRegions = ajaxification.cfg.ajaxRegionsSelector
          ? $(ajaxification.cfg.ajaxRegionsSelector, ajaxification.$replaceWrapper)
          : ajaxification.$replaceWrapper;

      // Go through all given regions and ajaxify them.
      $ajaxRegions.once('ajaxification-ajaxify').each(function() {
        var $region = $(this);

        $region.find('form input[type="submit"]').bind('mousedown click keydown', function() {
          var $this = $(this);
          $this.closest('form').find('input[type="submit"]').removeClass('ajaxification-input-submit');
          $this.addClass('ajaxification-input-submit');
        });

        // Prevent forms submit. Do it via ajax.
        $region.find('form').each(function() {
          $(this).once('ajaxification-submit', function() {
            var $form = $(this);
            var url = decodeURIComponent($form.attr('action'));
            if (ajaxification.checkUrl(url)) {
              // Apply our submit handler to form.
              $form.submit(function() {
                var data = $form.serializeArray();
                var $activeInputSubmit = $form.find('.ajaxification-input-submit');
                if ($activeInputSubmit.size()) {
                  var name = $activeInputSubmit.attr('name');
                  if (name !== '') {
                    var value = $activeInputSubmit.attr('value');
                    if (value !== '') {
                      data.push({name: name, value: value});
                    }
                  }
                }
                ajaxification.doAjax(url, $form.attr('method'), data, ajaxification.cfg.scrollToSelector);
                return false;
              });
              // Remove core ajax handlers from submit buttons. This can be
              // views-ajax handlers, etc.
              $form.find('input[type="submit"]').each(function() {
                $(this).once('ajaxification-prevent-other-ajax', function() {
                  var $submit = $(this);
                  if ($submit.data('events')) {
                    $.each($submit.data('events'), function(eventType, handlers) {
                      $.each(handlers, function(key, handler) {
                        if (handler.handler.toString().match(/return ajax\./gi)) {
                          $submit.unbind(eventType, handler.handler);
                        }
                      });
                    });
                  }
                });
              });
            }
          });
        });

        // Prevent links clicks. Do it via ajax.
        $region.find('a').once('ajaxification-click').click(function() {
          var $link = $(this);
          var url = decodeURIComponent($link.attr('href'));
          if (ajaxification.checkUrl(url)) {
            ajaxification.doAjax(url);
            return false;
          }
        });

        // Fix facetapi_select module behavior. Unbind original handler, and
        // bind own.
        if (Drupal.behaviors.facetapiSelect && Drupal.behaviors.facetapiSelect.goToLocation) {
          $('form', $region).unbind('change', Drupal.behaviors.facetapiSelect.goToLocation);
        }
        if (Drupal.settings.facetapi && Drupal.settings.facetapi.facets) {
          for (var index in Drupal.settings.facetapi.facets) {
            var facet = Drupal.settings.facetapi.facets[index];
            if (facet.widget === 'facetapi_select_dropdowns' && facet.autoSubmit === 1) {
              $('#' + facet.id + ' .form-select', $region).change(function() {
                var $this = $(this);
                var url = decodeURIComponent($this.val());
                if (ajaxification.checkUrl(url)) {
                  ajaxification.doAjax(url);
                  return false;
                }
              });
            }
          }
        }
      });
    }
  };

  // Fix facetapi's hard redirect.
  if (Drupal.facetapi && Drupal.facetapi.Redirect && Drupal.facetapi.Redirect.prototype.gotoHref) {
    var facetapiGotoHrefOrig = Drupal.facetapi.Redirect.prototype.gotoHref;
    Drupal.facetapi.Redirect.prototype.gotoHref = function() {
      if (ajaxification.checkUrl(this.href)) {
        ajaxification.doAjax(this.href);
        return false;
      }
      else {
        facetapiGotoHrefOrig();
      }
    }
  }

  /**
   * Initializes Drupal.ajaxification object and returns it.
   *
   * The purpose of the function: even if all the code from this file is parsed
   * on page load, we can prevent execution of the most part of it, if we don't
   * need it.
   */
  function initAjaxification() {
    Drupal.ajaxification = {

      /**
       * Wrapper of the main content that should be replace during ajax calls.
       *
       * Should be initialized outside.
       */
      $replaceWrapper: $(),

      /**
       * The first found appropriate configuration.
       *
       * See hook_ajaxification_configurations() function in
       * ajaxification.api.php.
       *
       * Should be initialized outside.
       */
      cfg: {
        replaceWrapperContextSelector: '',
        replaceWrapperSelector: '',
        ajaxRegionsSelector: '',
        historyApiOnly: '',
        basePaths: '',
        trackPageViewWithGA: '',
        reloadOnHistoryWalk: ''
      },

      /**
       * The currently  active configuration ID.
       */
      cfgId: '',

      /**
       * AXXX change it
       */
      $progress: $('<div class="ajax-progress ajax-progress-throbber" id="ajaxification-progress"><div class="throbber">&nbsp;</div></div>'),

      /**
       * An alias for window.history.pushState().
       */
      pushState: function(data,title,url) {
        if (window.history.pushState) {
          window.history.pushState(data,title,url);
        }
      },

      /**
       * Attaches handlers to history buttons.
       */
      handleHistoryButtons: function() {
        var self = this;
        window.addEventListener('popstate', function() {
          if (!self.history[window.location.href]) {
            return;
          }
          self.breakAjaxRequest();
          Drupal.settings = self.history[window.location.href].settings;
          if (self.cfg.reloadOnHistoryWalk) {
            self.doAjax(window.location.href, null, null, false);
          }
          else {
            self.restorePageState(window.location.href);
          }
        });
      },

      /**
       * Tracks page view with Google Analytics.
       */
      trackPageWithGA: function() {
        if (_gaq && _gaq.push) {
          _gaq.push(['_trackPageview']);
        }
      },

      /**
       * Current ajax request object.
       */
      request: false,

      /**
       * Breaks AJAX request, if there is one.
       */
      breakAjaxRequest: function() {
        var self = this;
        if (typeof(self.request) == 'object') {
          self.request.abort();
          self.request = false;
        }
        self.hideProgress();
      },

      /**
       * Contains page states keyed by URLs.
       */
      history: {},

      /**
       * Saves page state.
       */
      savePageState: function(url, providedData) {
        var self = this;
        providedData = providedData || {};
        var data = {};
        data.content = providedData.content || self.$replaceWrapper.html();
        data.settings = providedData.settings || Drupal.settings;
        data.pageTitle = settings.pageTitle || $('head title').html();
        if (providedData.h1) {
          data.h1 = providedData.h1;
        }
        else {
          var $h1 = $('h1');
          if ($h1.size() == 1) {
            data.h1 = $h1.html();
          }
        }
        data.bodyClass = providedData.bodyClass || $('body').attr('class');
        self.history[url] = data;
      },

      /**
       * Restores page state.
       */
      restorePageState: function(url) {
        var self = this;
        if (!self.history[url]) {
          return;
        }

        Drupal.freezeHeight();

        Drupal.settings = self.history[url].settings;
        self.$replaceWrapper.html(self.history[url].content);
        $('head title').html(self.history[url].pageTitle);
        if (self.history[url].h1) {
          var $h1 = $('h1');
          if ($h1.size() == 1) {
            $h1.html(self.history[url].h1);
          }
        }
        $('body').attr('class', self.history[url].bodyClass);

        self.$replaceWrapper.removeClass('ajaxification-ajaxify-processed');
        Drupal.attachBehaviors(self.$replaceWrapper, Drupal.settings);
        if (self.cfg.trackPageViewWithGA) {
          self.trackPageWithGA();
        }

        Drupal.unfreezeHeight();
      },

      /**
       * Checks if URL could be ajaxified.
       */
      checkUrl: function (url) {
        var self = this;
        var ret = false;
        url = url.toLowerCase();
        $.each(self.urlVariants(self.cfg.basePaths), function(_, variant) {
          if (url == variant
              || url.indexOf(variant + '/') == 0
              || url.indexOf(variant + '?') == 0
              || url.indexOf(variant + '#') == 0) {
            ret = true;
            return false;
          }
        });
        return ret;
      },
      urlVariants: function(basePaths) {
        var variants = [];
        $.each(basePaths, function(_, basePath) {
          var domain_port = document.domain + (location.port ? ':' + location.port : '');
          var _variants = [
            'http://' + domain_port + Drupal.settings.basePath + basePath,
            'http://' + domain_port + Drupal.settings.basePath + Drupal.settings.pathPrefix + basePath,
            'https://' + domain_port + Drupal.settings.basePath + basePath,
            'https://' + domain_port + Drupal.settings.basePath + Drupal.settings.pathPrefix + basePath,
            Drupal.settings.basePath + basePath,
            Drupal.settings.basePath + Drupal.settings.pathPrefix + basePath
          ];
          $.each(_variants, function(_, _variant) {
            variants.push(_variant.replace(/\/+$/,'').toLowerCase());
          });
        });
        return variants;
      },

      /**
       * Performs AJAX request. Request type and data are optional.
       */
      doAjax: function(url, type, data, pushState) {
        var self = this;
        pushState = (typeof(pushState) == 'undefined') ? true : pushState;
        self.breakAjaxRequest();
        self.showProgress();
        type = type || 'get';
        data = data || [];

        // Indicate ajaxification call.
        data.push({name: 'ajaxification_cfg_id', value: self.cfgId});

        // Add ajaxPageState.
        var state = [];
        state.push(Drupal.settings.ajaxPageState.theme);
        state.push(Drupal.settings.ajaxPageState.theme_token);
        for (var resourceType in {js: '', css: ''}) {
          for (var key in Drupal.settings.ajaxPageState[resourceType]) {
            if (Drupal.settings.ajaxification.resource_to_id_map[resourceType][key]) {
              state.push(Drupal.settings.ajaxification.resource_to_id_map[resourceType][key]);
            }
          }
        }
        data.push({name: 'ajaxification_page_state', value: state.join('/')});

        self.request = $.ajax({
          url: url,
          type: type,
          data: data,
          success: function(response) {
            if (pushState) {
              self.pushState(null, '', response.url);
            }

            for (var i in response.ajax_commands) {
              if (response.ajax_commands.hasOwnProperty(i) && response.ajax_commands[i]['command'] && self.ajax.commands[response.ajax_commands[i]['command']]) {
                self.ajax.commands[response.ajax_commands[i]['command']](self.ajax, response.ajax_commands[i]);
              }
            }

            self.savePageState(response.url, response);
            self.restorePageState(response.url);
            self.hideProgress();

            if (self.cfg.scrollToSelector) {
              var offset = $(self.cfg.scrollToSelector).eq(0).offset();
              if (offset && offset.top) {
                $('html, body').animate({scrollTop: offset.top}, 'fast');
              }
            }
          }
          //AXXX handle errors
        });
      },

      //AXXX doc
      showProgress: function() {
        var self = this;
        $('body').append(self.$progress);
      },

      //AXXX doc
      hideProgress: function() {
        $('#ajaxification-progress').remove();
      },

      ajax: $.extend({
        url: 'system/ajax',
        event: 'mousedown',
        keypress: true,
        selector: '',
        effect: 'none',
        speed: 'none',
        method: 'replaceWith',
        progress: {
          type: 'throbber',
          message: Drupal.t('Please wait...')
        },
        submit: {
          'js': true
        }
      }, Drupal.ajax.prototype)
    };

    return Drupal.ajaxification;
  }

})(jQuery);