#ifndef ADAPTER_INTERNAL_H
#define ADAPTER_INTERNAL_H

#include "acrefxadapter.h"

struct _AcreFxAddon {
    gchar *id;
    gchar *path;
    AcreFxManifest *manifest;
    gboolean enabled;
    GHashTable *scripts; /* content scripts */
    gpointer background_handle;
    gboolean background_loaded;
};

struct _AcreFxAdapter {
    WebKitWebContext *context;
    GHashTable *addons; /* addon_id -> AcreFxAddon */
    GPtrArray *loaded_paths;
    gpointer message_bus;
    gchar *config_dir;
};

AcreFxManifest *acrefx_manifest_parse(const gchar *manifest_path);
void acrefx_manifest_free(AcreFxManifest *manifest);
gboolean acrefx_manifest_is_compatible(AcreFxManifest *manifest);

#endif /* ADAPTER_INTERNAL_H */

/* Match pattern functions */
gboolean acrefx_match_pattern_matches(const gchar *pattern, const gchar *url);
gboolean acrefx_should_inject_script(JsonObject *cs_obj, const gchar *url);
WebKitUserScriptInjectionTime acrefx_get_injection_time(const gchar *run_at);
gboolean acrefx_is_safe_path(const gchar *base_path, const gchar *user_path);
