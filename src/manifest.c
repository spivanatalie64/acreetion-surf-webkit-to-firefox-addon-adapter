#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "acrefxadapter.h"
#include "adapter_internal.h"

AcreFxManifest *acrefx_manifest_parse(const gchar *manifest_path) {
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    
    if (!json_parser_load_from_file(parser, manifest_path, &error)) {
        g_warning("Failed to parse manifest: %s", error ? error->message : "unknown error");
        g_clear_error(&error);
        g_object_unref(parser);
        return NULL;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return NULL;
    }
    
    JsonObject *obj = json_node_get_object(root);
    AcreFxManifest *manifest = g_new0(AcreFxManifest, 1);
    
    if (json_object_has_member(obj, "manifest_version")) {
        manifest->manifest_version = g_strdup(json_object_get_string_member(obj, "manifest_version"));
    }
    if (json_object_has_member(obj, "name")) {
        manifest->name = g_strdup(json_object_get_string_member(obj, "name"));
    }
    if (json_object_has_member(obj, "version")) {
        manifest->version = g_strdup(json_object_get_string_member(obj, "version"));
    }
    if (json_object_has_member(obj, "description")) {
        manifest->description = g_strdup(json_object_get_string_member(obj, "description"));
    }
    if (json_object_has_member(obj, "browser_action")) {
        JsonObject *browser_action = json_object_get_object_member(obj, "browser_action");
        if (json_object_has_member(browser_action, "default_popup")) {
            manifest->browser_action_default_popup = g_strdup(json_object_get_string_member(browser_action, "default_popup"));
        }
    }
    if (json_object_has_member(obj, "options_page")) {
        manifest->options_page = g_strdup(json_object_get_string_member(obj, "options_page"));
    }
    if (json_object_has_member(obj, "background")) {
        JsonObject *background = json_object_get_object_member(obj, "background");
        if (json_object_has_member(background, "scripts")) {
            JsonArray *scripts = json_object_get_array_member(background, "scripts");
            if (json_array_get_length(scripts) > 0) {
                manifest->background_script = g_strdup(json_array_get_string_element(scripts, 0));
            }
        } else if (json_object_has_member(background, "service_worker")) {
            /* MV3 - handle later */
        }
    }
    
    manifest->permissions = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "permissions")) {
        JsonArray *perms = json_object_get_array_member(obj, "permissions");
        for (guint i = 0; i < json_array_get_length(perms); i++) {
            const gchar *perm = json_array_get_string_element(perms, i);
            g_ptr_array_add(manifest->permissions, g_strdup(perm));
        }
    }
    
    manifest->host_permissions = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "host_permissions")) {
        JsonArray *hosts = json_object_get_array_member(obj, "host_permissions");
        for (guint i = 0; i < json_array_get_length(hosts); i++) {
            const gchar *host = json_array_get_string_element(hosts, i);
            g_ptr_array_add(manifest->host_permissions, g_strdup(host));
        }
    }
    
    manifest->content_scripts = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "content_scripts")) {
        JsonArray *cs = json_object_get_array_member(obj, "content_scripts");
        for (guint i = 0; i < json_array_get_length(cs); i++) {
            JsonNode *cs_node = json_array_get_element(cs, i);
            if (JSON_NODE_HOLDS_OBJECT(cs_node)) {
                JsonObject *cs_obj = json_node_get_object(cs_node);
                gchar *cs_str = json_to_string(cs_node, FALSE);
                g_ptr_array_add(manifest->content_scripts, cs_str);
            }
        }
    }
    
    g_object_unref(parser);
    return manifest;
}

void acrefx_manifest_free(AcreFxManifest *manifest) {
    if (!manifest) return;
    g_free(manifest->manifest_version);
    g_free(manifest->name);
    g_free(manifest->version);
    g_free(manifest->description);
    g_free(manifest->background_script);
    g_free(manifest->browser_action_default_popup);
    g_free(manifest->options_page);
    if (manifest->permissions) g_ptr_array_unref(manifest->permissions);
    if (manifest->host_permissions) g_ptr_array_unref(manifest->host_permissions);
    if (manifest->content_scripts) g_ptr_array_unref(manifest->content_scripts);
    g_free(manifest);
}
