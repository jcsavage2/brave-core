// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'brave-sync-setup' is the UI for starting or joining a sync chain
 * settings.
 */
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/
Polymer({
  is: 'settings-p2p-keys-manager',

  _template: html`{__html_template__}`,

  behaviors: [

  ],

  properties: {
    /**
    * Current page status
    * 'configure' | 'setup' | 'spinner'
    * @private
    */
    pageStatus_: {
      type: String,
      value: 'configure',
    },

  },

  /** @private {?SyncBrowserProxy} */
  browserProxy_: null,

  /**
  * The beforeunload callback is used to show the 'Leave site' dialog. This
  * makes sure that the user has the chance to go back and confirm the sync
  * opt-in before leaving.
  *
  * This property is non-null if the user is currently navigated on the sync
  * settings route.
  *
  * @private {?Function}
  */
  beforeunloadCallback_: null,

  /**
  * The unload callback is used to cancel the sync setup when the user hits
  * the browser back button after arriving on the page.
  * Note: Cases like closing the tab or reloading don't need to be handled,
  * because they are already caught in |PeopleHandler::~PeopleHandler|
  * from the C++ code.
  *
  * @private {?Function}
  */
  unloadCallback_: null,

  /**
  * Whether the user completed setup successfully.
  * @private {boolean}
  */
  setupSuccessful_: false,

  /** @override */
  created: function() {
    console.log(11);
    // this.browserProxy_ = SyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    console.log(11);
    const router = Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_KEYS) {
      this.onNavigateToPage_();
    }
  },

  /** @override */
  detached: function() {
    const router = Router.getInstance();
    if (router.getRoutes().BRAVE_IPFS_KEYS.contains(router.getCurrentRoute())) {
      this.onNavigateAwayFromPage_();
    }

    if (this.beforeunloadCallback_) {
      window.removeEventListener('beforeunload', this.beforeunloadCallback_);
      this.beforeunloadCallback_ = null;
    }
    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_);
      this.unloadCallback_ = null;
    }
  },

  updatePageStatus_: function () {
    //const isFirstSetup = this.syncStatus && this.syncStatus.firstSetupInProgress
    //this.pageStatus_ = isFirstSetup ? 'setup' : 'configure'
  },

  /** @protected */
  currentRouteChanged: function() {
    const router = Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_KEYS) {
      this.onNavigateToPage_();
      return;
    }

    if (router.getRoutes().BRAVE_IPFS_KEYS.contains(router.getCurrentRoute())) {
      return;
    }

    this.onNavigateAwayFromPage_();
  },

  /**
  * @param {!PageStatus} expectedPageStatus
  * @return {boolean}
  * @private
  */
  isStatus_: function(expectedPageStatus) {
    return expectedPageStatus == this.pageStatus_;
  },

  /** @private */
  onNavigateToPage_: function() {
    const router = Router.getInstance();
    assert(router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_KEYS);
    if (this.beforeunloadCallback_) {
      return;
    }

    // Triggers push of prefs to our handler
    //if (this.browserProxy_)
      //this.browserProxy_.didNavigateToSyncPage();

    this.unloadCallback_ = this.onNavigateAwayFromPage_.bind(this);
    window.addEventListener('unload', this.unloadCallback_);
  },

  /** @private */
  onNavigateAwayFromPage_: function() {
    if (!this.beforeunloadCallback_) {
      return;
    }

    this.browserProxy_.didNavigateAwayFromSyncPage(!this.setupSuccessful_);

    // Reset state as this component could actually be kept around even though
    // it is hidden.
    this.setupSuccessful_ = false

    window.removeEventListener('beforeunload', this.beforeunloadCallback_);
    this.beforeunloadCallback_ = null;

    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_);
      this.unloadCallback_ = null;
    }
  },

  /**
  * Called when setup is complete and sync code is set
  * @private
  */
  onSetupSuccess_: function() {
    this.setupSuccessful_ = true
    // This navigation causes the firstSetupInProgress flag to be marked as false
    // via `didNavigateAwayFromSyncPage`.
    const router = Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_KEYS) {
      router.navigateTo(router.getRoutes().BRAVE_IPFS);
    }
  },
});