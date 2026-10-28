#ifndef ACREFXADAPTER_H
#define ACREFXADAPTER_H

#include <glib.h>
#include <webkit2/webkit2.h>
#include <json-glib/json-glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct _AcreFxAdapter AcreFxAdapter;
typedef struct _AcreFxAddon AcreFxAddon;

/* Manifest structure */
typedef struct {
    gchar *manifest_version;
    gint manifest_version_int;
    gboolean is_manifest_v3;
    
    gchar *name;
    gchar *version;
    gchar *description;
    
    GPtrArray *permissions;
    GPtrArray *host_permissions;
    GPtrArray *optional_permissions;
    GPtrArray *content_scripts;
    
    /* Background */
    gchar *background_script;
    gchar *background_page;
    gchar *background_service_worker;
    gchar *background_type; /* "scripts", "page", "service_worker" */
    
    /* Action/Browser Action */
    gchar *browser_action_default_popup;
    gchar *page_action_default_popup;
    gchar *action_default_popup;
    gchar *default_icon;
    
    gchar *options_page;
    gchar *options_ui_page;
    gchar *devtools_page;
    
    /* MV3 specific */
    GPtrArray *web_accessible_resources; /* can be complex in MV3 */
} AcreFxManifest;

/* Adapter main object */
AcreFxAdapter *acrefx_adapter_new(WebKitWebContext *context);
void acrefx_adapter_free(AcreFxAdapter *adapter);

/* Load addon from directory or xpi */
gboolean acrefx_adapter_load_addon(AcreFxAdapter *adapter, const gchar *addon_path);
gboolean acrefx_adapter_load_addon_from_xpi(AcreFxAdapter *adapter, const gchar *xpi_path);

/* Enable/disable addons */
gboolean acrefx_adapter_enable_addon(AcreFxAdapter *adapter, const gchar *addon_id, gboolean enable);
void acrefx_adapter_reload_addons(AcreFxAdapter *adapter);

/* Get loaded addons */
GList *acrefx_adapter_get_addons(AcreFxAdapter *adapter);

/* Content script injection */
void acrefx_adapter_inject_content_scripts(AcreFxAdapter *adapter, WebKitWebView *webview);

/* Message passing */
void acrefx_adapter_send_message_to_content(AcreFxAdapter *adapter, const gchar *addon_id, 
                                            const gchar *message, const gchar *json_data);
void acrefx_adapter_broadcast_message(AcreFxAdapter *adapter, const gchar *message, const gchar *json_data);

/* API bridge initialization */
void acrefx_adapter_init_webextension_api(WebKitWebView *webview, AcreFxAddon *addon);

#ifdef __cplusplus
}
#endif

#endif /* ACREFXADAPTER_H */
