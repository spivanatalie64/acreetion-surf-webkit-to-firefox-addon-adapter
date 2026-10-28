#define _GNU_SOURCE
#include <webkit2/webkit2.h>
#include "acrefxadapter.h"
#include "adapter_internal.h"

static const gchar *CONTENT_BRIDGE_JS = 
    "(function(){\n"
    "  if(window.__acrefx_injected) return;\n"
    "  window.__acrefx_injected = true;\n"
    "  window.browser = window.browser || {};\n"
    "  window.chrome = window.chrome || window.browser;\n"
    "  console.log('[acrefx] webextension API bridge loaded');\n"
    "})();";

void acrefx_inject_content_bridge(WebKitWebView *webview) {
    webkit_web_view_run_javascript(webview, CONTENT_BRIDGE_JS, NULL, NULL, NULL);
}
