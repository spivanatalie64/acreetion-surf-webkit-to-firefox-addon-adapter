#define _GNU_SOURCE
#include <webkit2/webkit2.h>
#include <json-glib/json-glib.h>
#include "acrefxadapter.h"
#include "adapter_internal.h"

static const gchar *API_BRIDGE_JS =
    "(function(){\n"
    "  if(window.__acrefx_api) return;\n"
    "  window.__acrefx_api = {};\n"
    "  \n"
    "  // browser.runtime\n"
    "  window.browser = window.browser || {};\n"
    "  window.browser.runtime = window.browser.runtime || {};\n"
    "  window.browser.runtime.sendMessage = function(extensionId, message, options, callback){\n"
    "    // will be bridged by native code\n"
    "    return Promise.resolve({});\n"
    "  };\n"
    "  window.browser.runtime.onMessage = {\n"
    "    addListener: function(fn){ this._listeners = this._listeners || []; this._listeners.push(fn); }\n"
    "  };\n"
    "  \n"
    "  // browser.tabs\n"
    "  window.browser.tabs = window.browser.tabs || {};\n"
    "  window.browser.tabs.query = function(queryInfo, callback){\n"
    "    return Promise.resolve([]);\n"
    "  };\n"
    "  window.browser.tabs.sendMessage = function(tabId, message, options, callback){\n"
    "    return Promise.resolve({});\n"
    "  };\n"
    "  \n"
    "  // browser.storage\n"
    "  window.browser.storage = window.browser.storage || {};\n"
    "  window.browser.storage.local = {\n"
    "    get: function(keys, callback){ if(callback) callback({}); return Promise.resolve({}); },\n"
    "    set: function(items, callback){ if(callback) callback(); return Promise.resolve(); },\n"
    "    remove: function(keys, callback){ if(callback) callback(); return Promise.resolve(); },\n"
    "    clear: function(callback){ if(callback) callback(); return Promise.resolve(); }\n"
    "  };\n"
    "  \n"
    "  console.log('[acrefx] browser API polyfill installed');\n"
    "})();";

void acrefx_adapter_init_webextension_api(WebKitWebView *webview, AcreFxAddon *addon) {
    (void)addon;
    webkit_web_view_run_javascript(webview, API_BRIDGE_JS, NULL, NULL, NULL);
}
