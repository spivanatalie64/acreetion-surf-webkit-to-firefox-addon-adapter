#define _GNU_SOURCE
#include <webkit2/webkit2.h>
#include <json-glib/json-glib.h>
#include <string.h>
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
    if (!webview) return;
    webkit_web_view_run_javascript(webview, CONTENT_BRIDGE_JS, NULL, NULL, NULL);
}

void acrefx_adapter_inject_content_scripts(AcreFxAdapter *adapter, WebKitWebView *webview) {
    if (!adapter || !webview) return;
    
    const gchar *current_uri = webkit_web_view_get_uri(webview);
    if (!current_uri) return;
    
    WebKitUserContentManager *ucm = webkit_web_view_get_user_content_manager(webview);
    if (!ucm) return;
    
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, adapter->addons);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        AcreFxAddon *addon = value;
        if (!addon->enabled || !addon->manifest) continue;
        if (!addon->manifest->content_scripts) continue;
        
        for (guint i = 0; i < addon->manifest->content_scripts->len; i++) {
            gchar *cs_json = g_ptr_array_index(addon->manifest->content_scripts, i);
            JsonParser *parser = json_parser_new();
            GError *error = NULL;
            
            if (!json_parser_load_from_data(parser, cs_json, -1, &error)) {
                g_clear_error(&error);
                g_object_unref(parser);
                continue;
            }
            
            JsonNode *root = json_parser_get_root(parser);
            if (!JSON_NODE_HOLDS_OBJECT(root)) {
                g_object_unref(parser);
                continue;
            }
            
            JsonObject *cs_obj = json_node_get_object(root);
            
            /* Check match patterns */
            if (!acrefx_should_inject_script(cs_obj, current_uri)) {
                g_object_unref(parser);
                continue;
            }
            
            /* Get injection time */
            const gchar *run_at = NULL;
            if (json_object_has_member(cs_obj, "run_at")) {
                run_at = json_object_get_string_member(cs_obj, "run_at");
            }
            WebKitUserScriptInjectionTime injection_time = acrefx_get_injection_time(run_at);
            
            GPtrArray *js_files = g_ptr_array_new_with_free_func(g_free);
            GPtrArray *css_files = g_ptr_array_new_with_free_func(g_free);
            
            /* Collect JS files */
            if (json_object_has_member(cs_obj, "js")) {
                JsonArray *js_arr = json_object_get_array_member(cs_obj, "js");
                for (guint j = 0; j < json_array_get_length(js_arr); j++) {
                    const gchar *js_file = json_array_get_string_element(js_arr, j);
                    if (js_file && acrefx_is_safe_path(addon->path, js_file)) {
                        gchar *full_path = g_build_filename(addon->path, js_file, NULL);
                        g_ptr_array_add(js_files, full_path);
                    } else if (js_file) {
                        g_warning("[acrefx] Unsafe path rejected: %s", js_file);
                    }
                }
            }
            
            /* Collect CSS files */
            if (json_object_has_member(cs_obj, "css")) {
                JsonArray *css_arr = json_object_get_array_member(cs_obj, "css");
                for (guint j = 0; j < json_array_get_length(css_arr); j++) {
                    const gchar *css_file = json_array_get_string_element(css_arr, j);
                    if (css_file && acrefx_is_safe_path(addon->path, css_file)) {
                        gchar *full_path = g_build_filename(addon->path, css_file, NULL);
                        g_ptr_array_add(css_files, full_path);
                    } else if (css_file) {
                        g_warning("[acrefx] Unsafe path rejected: %s", css_file);
                    }
                }
            }
            
            /* Read and inject CSS */
            for (guint j = 0; j < css_files->len; j++) {
                gchar *css_path = g_ptr_array_index(css_files, j);
                gchar *css_content = NULL;
                gsize css_len = 0;
                if (g_file_get_contents(css_path, &css_content, &css_len, NULL) && css_content) {
                    /* Limit file size to 10MB */
                    if (css_len > 10 * 1024 * 1024) {
                        g_warning("[acrefx] CSS file too large: %s", css_path);
                        g_free(css_content);
                        continue;
                    }
                    WebKitUserStyleSheet *style = webkit_user_style_sheet_new(
                        css_content,
                        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                        WEBKIT_USER_STYLE_LEVEL_USER,
                        NULL,
                        NULL
                    );
                    webkit_user_content_manager_add_style_sheet(ucm, style);
                    g_free(css_content);
                }
            }
            
            /* Read and inject JS */
            for (guint j = 0; j < js_files->len; j++) {
                gchar *js_path = g_ptr_array_index(js_files, j);
                gchar *js_content = NULL;
                gsize js_len = 0;
                if (g_file_get_contents(js_path, &js_content, &js_len, NULL) && js_content) {
                    /* Limit file size to 10MB */
                    if (js_len > 10 * 1024 * 1024) {
                        g_warning("[acrefx] JS file too large: %s", js_path);
                        g_free(js_content);
                        continue;
                    }
                    /* Prepend API bridge */
                    gchar *full_script = g_strconcat(CONTENT_BRIDGE_JS, "\n;\n", js_content, NULL);
                    WebKitUserScript *script = webkit_user_script_new(
                        full_script,
                        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                        injection_time,
                        NULL,
                        NULL
                    );
                    webkit_user_content_manager_add_script(ucm, script);
                    g_free(full_script);
                    g_free(js_content);
                }
            }
            
            g_ptr_array_unref(js_files);
            g_ptr_array_unref(css_files);
            g_object_unref(parser);
        }
    }
}
