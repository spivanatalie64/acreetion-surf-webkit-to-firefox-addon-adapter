#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <webkit2/webkit2.h>
#include "acrefxadapter.h"
#include "adapter_internal.h"

AcreFxAdapter *acrefx_adapter_new(WebKitWebContext *context) {
    AcreFxAdapter *adapter = g_new0(AcreFxAdapter, 1);
    adapter->context = context;
    adapter->addons = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    adapter->loaded_paths = g_ptr_array_new_with_free_func(g_free);
    return adapter;
}

void acrefx_adapter_free(AcreFxAdapter *adapter) {
    if (!adapter) return;
    if (adapter->addons) {
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, adapter->addons);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            AcreFxAddon *addon = value;
            /* cleanup addon */
            g_free(addon->id);
            g_free(addon->path);
            if (addon->manifest) acrefx_manifest_free(addon->manifest);
            g_free(addon);
        }
        g_hash_table_unref(adapter->addons);
    }
    if (adapter->loaded_paths) g_ptr_array_unref(adapter->loaded_paths);
    g_free(adapter);
}

gboolean acrefx_adapter_load_addon(AcreFxAdapter *adapter, const gchar *addon_path) {
    if (!adapter || !addon_path) return FALSE;
    
    gchar *manifest_path = g_build_filename(addon_path, "manifest.json", NULL);
    struct stat st;
    if (stat(manifest_path, &st) != 0) {
        g_free(manifest_path);
        return FALSE;
    }
    
    AcreFxManifest *manifest = acrefx_manifest_parse(manifest_path);
    g_free(manifest_path);
    if (!manifest) return FALSE;
    
    AcreFxAddon *addon = g_new0(AcreFxAddon, 1);
    addon->id = g_strdup_printf("%s@acreetion", manifest->name ? manifest->name : "addon");
    /* normalize to a proper id */
    gchar *clean_id = g_strdup(addon->id);
    for (gchar *p = clean_id; *p; p++) {
        if (!g_ascii_isalnum(*p) && *p != '@' && *p != '.' && *p != '-' && *p != '_') *p = '_';
    }
    g_free(addon->id);
    addon->id = clean_id;
    
    addon->path = g_strdup(addon_path);
    addon->manifest = manifest;
    addon->enabled = TRUE;
    
    g_hash_table_insert(adapter->addons, g_strdup(addon->id), addon);
    g_ptr_array_add(adapter->loaded_paths, g_strdup(addon_path));
    
    g_message("Loaded addon: %s (%s)", manifest->name, addon->id);
    return TRUE;
}

gboolean acrefx_adapter_load_addon_from_xpi(AcreFxAdapter *adapter, const gchar *xpi_path) {
    /* TODO: extract xpi (zip) and load */
    (void)adapter; (void)xpi_path;
    return FALSE;
}

gboolean acrefx_adapter_enable_addon(AcreFxAdapter *adapter, const gchar *addon_id, gboolean enable) {
    if (!adapter || !addon_id) return FALSE;
    AcreFxAddon *addon = g_hash_table_lookup(adapter->addons, addon_id);
    if (!addon) return FALSE;
    addon->enabled = enable;
    return TRUE;
}

void acrefx_adapter_reload_addons(AcreFxAdapter *adapter) {
    (void)adapter;
}

GList *acrefx_adapter_get_addons(AcreFxAdapter *adapter) {
    if (!adapter) return NULL;
    return g_hash_table_get_keys(adapter->addons);
}

void acrefx_adapter_inject_content_scripts(AcreFxAdapter *adapter, WebKitWebView *webview) {
    if (!adapter || !webview) return;
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, adapter->addons);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        AcreFxAddon *addon = value;
        if (!addon->enabled || !addon->manifest) continue;
        /* TODO: parse content_scripts matches and inject */
        (void)addon;
    }
}

void acrefx_adapter_send_message_to_content(AcreFxAdapter *adapter, const gchar *addon_id,
                                            const gchar *message, const gchar *json_data) {
    (void)adapter; (void)addon_id; (void)message; (void)json_data;
}

void acrefx_adapter_broadcast_message(AcreFxAdapter *adapter, const gchar *message, const gchar *json_data) {
    (void)adapter; (void)message; (void)json_data;
}

void acrefx_adapter_init_webextension_api(WebKitWebView *webview, AcreFxAddon *addon) {
    (void)webview; (void)addon;
}
