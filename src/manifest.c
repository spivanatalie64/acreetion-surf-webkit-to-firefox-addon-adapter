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
    
    /* Manifest version */
    if (json_object_has_member(obj, "manifest_version")) {
        manifest->manifest_version = g_strdup(json_object_get_string_member(obj, "manifest_version"));
        manifest->manifest_version_int = atoi(manifest->manifest_version);
    } else {
        /* Default to v2 for compatibility */
        manifest->manifest_version = g_strdup("2");
        manifest->manifest_version_int = 2;
    }
    manifest->is_manifest_v3 = (manifest->manifest_version_int == 3);
    
    if (json_object_has_member(obj, "name")) {
        manifest->name = g_strdup(json_object_get_string_member(obj, "name"));
    }
    if (json_object_has_member(obj, "version")) {
        manifest->version = g_strdup(json_object_get_string_member(obj, "version"));
    }
    if (json_object_has_member(obj, "description")) {
        manifest->description = g_strdup(json_object_get_string_member(obj, "description"));
    }
    
    /* Browser/Action buttons - handle both v2 and v3 */
    if (json_object_has_member(obj, "browser_action")) {
        JsonObject *browser_action = json_object_get_object_member(obj, "browser_action");
        if (json_object_has_member(browser_action, "default_popup")) {
            manifest->browser_action_default_popup = g_strdup(json_object_get_string_member(browser_action, "default_popup"));
        }
        if (json_object_has_member(browser_action, "default_icon")) {
            /* Could be string or object, handle simply */
            if (JSON_NODE_HOLDS_VALUE(json_object_get_member(browser_action, "default_icon"))) {
                manifest->default_icon = g_strdup(json_object_get_string_member(browser_action, "default_icon"));
            }
        }
    }
    if (json_object_has_member(obj, "page_action")) {
        JsonObject *page_action = json_object_get_object_member(obj, "page_action");
        if (json_object_has_member(page_action, "default_popup")) {
            manifest->page_action_default_popup = g_strdup(json_object_get_string_member(page_action, "default_popup"));
        }
    }
    if (json_object_has_member(obj, "action")) {
        /* Manifest V3 */
        JsonObject *action = json_object_get_object_member(obj, "action");
        if (json_object_has_member(action, "default_popup")) {
            manifest->action_default_popup = g_strdup(json_object_get_string_member(action, "default_popup"));
        }
    }
    
    /* Options pages */
    if (json_object_has_member(obj, "options_page")) {
        manifest->options_page = g_strdup(json_object_get_string_member(obj, "options_page"));
    }
    if (json_object_has_member(obj, "options_ui")) {
        JsonObject *options_ui = json_object_get_object_member(obj, "options_ui");
        if (json_object_has_member(options_ui, "page")) {
            manifest->options_ui_page = g_strdup(json_object_get_string_member(options_ui, "page"));
        }
    }
    if (json_object_has_member(obj, "devtools_page")) {
        manifest->devtools_page = g_strdup(json_object_get_string_member(obj, "devtools_page"));
    }
    
    /* Background scripts - handle both v2 and v3 */
    if (json_object_has_member(obj, "background")) {
        JsonObject *background = json_object_get_object_member(obj, "background");
        if (json_object_has_member(background, "scripts")) {
            /* Manifest V2 */
            JsonArray *scripts = json_object_get_array_member(background, "scripts");
            if (json_array_get_length(scripts) > 0) {
                manifest->background_script = g_strdup(json_array_get_string_element(scripts, 0));
                manifest->background_type = g_strdup("scripts");
            }
        } else if (json_object_has_member(background, "page")) {
            /* Manifest V2 with background page */
            manifest->background_page = g_strdup(json_object_get_string_member(background, "page"));
            manifest->background_type = g_strdup("page");
        } else if (json_object_has_member(background, "service_worker")) {
            /* Manifest V3 */
            manifest->background_service_worker = g_strdup(json_object_get_string_member(background, "service_worker"));
            manifest->background_type = g_strdup("service_worker");
        }
    }
    
    /* Permissions */
    manifest->permissions = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "permissions")) {
        JsonArray *perms = json_object_get_array_member(obj, "permissions");
        for (guint i = 0; i < json_array_get_length(perms); i++) {
            JsonNode *node = json_array_get_element(perms, i);
            if (JSON_NODE_HOLDS_VALUE(node)) {
                const gchar *perm = json_array_get_string_element(perms, i);
                if (perm) {
                    g_ptr_array_add(manifest->permissions, g_strdup(perm));
                }
            }
        }
    }
    
    /* Host permissions (MV3) */
    manifest->host_permissions = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "host_permissions")) {
        JsonArray *hosts = json_object_get_array_member(obj, "host_permissions");
        for (guint i = 0; i < json_array_get_length(hosts); i++) {
            JsonNode *node = json_array_get_element(hosts, i);
            if (JSON_NODE_HOLDS_VALUE(node)) {
                const gchar *host = json_array_get_string_element(hosts, i);
                if (host) {
                    g_ptr_array_add(manifest->host_permissions, g_strdup(host));
                }
            }
        }
    }
    
    /* Optional permissions */
    manifest->optional_permissions = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "optional_permissions")) {
        JsonArray *opts = json_object_get_array_member(obj, "optional_permissions");
        for (guint i = 0; i < json_array_get_length(opts); i++) {
            JsonNode *node = json_array_get_element(opts, i);
            if (JSON_NODE_HOLDS_VALUE(node)) {
                const gchar *perm = json_array_get_string_element(opts, i);
                if (perm) {
                    g_ptr_array_add(manifest->optional_permissions, g_strdup(perm));
                }
            }
        }
    }
    
    /* Content scripts */
    manifest->content_scripts = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "content_scripts")) {
        JsonArray *cs = json_object_get_array_member(obj, "content_scripts");
        for (guint i = 0; i < json_array_get_length(cs); i++) {
            JsonNode *cs_node = json_array_get_element(cs, i);
            if (JSON_NODE_HOLDS_OBJECT(cs_node)) {
                gchar *cs_str = json_to_string(cs_node, FALSE);
                g_ptr_array_add(manifest->content_scripts, cs_str);
            }
        }
    }
    
    /* Web accessible resources (MV3) */
    manifest->web_accessible_resources = g_ptr_array_new_with_free_func(g_free);
    if (json_object_has_member(obj, "web_accessible_resources")) {
        JsonNode *war_node = json_object_get_member(obj, "web_accessible_resources");
        if (JSON_NODE_HOLDS_ARRAY(war_node)) {
            JsonArray *war = json_node_get_array(war_node);
            for (guint i = 0; i < json_array_get_length(war); i++) {
                gchar *war_str = json_to_string(json_array_get_element(war, i), FALSE);
                g_ptr_array_add(manifest->web_accessible_resources, war_str);
            }
        }
    }
    
    g_object_unref(parser);
    return manifest;
}

gboolean acrefx_manifest_is_compatible(AcreFxManifest *manifest) {
    if (!manifest) return FALSE;
    /* Support both v2 and v3 */
    if (manifest->manifest_version_int == 2 || manifest->manifest_version_int == 3) {
        return TRUE;
    }
    return FALSE;
}

void acrefx_manifest_free(AcreFxManifest *manifest) {
    if (!manifest) return;
    g_free(manifest->manifest_version);
    g_free(manifest->name);
    g_free(manifest->version);
    g_free(manifest->description);
    g_free(manifest->background_script);
    g_free(manifest->background_page);
    g_free(manifest->background_service_worker);
    g_free(manifest->background_type);
    g_free(manifest->browser_action_default_popup);
    g_free(manifest->page_action_default_popup);
    g_free(manifest->action_default_popup);
    g_free(manifest->default_icon);
    g_free(manifest->options_page);
    g_free(manifest->options_ui_page);
    g_free(manifest->devtools_page);
    if (manifest->permissions) g_ptr_array_unref(manifest->permissions);
    if (manifest->host_permissions) g_ptr_array_unref(manifest->host_permissions);
    if (manifest->optional_permissions) g_ptr_array_unref(manifest->optional_permissions);
    if (manifest->content_scripts) g_ptr_array_unref(manifest->content_scripts);
    if (manifest->web_accessible_resources) g_ptr_array_unref(manifest->web_accessible_resources);
    g_free(manifest);
}
