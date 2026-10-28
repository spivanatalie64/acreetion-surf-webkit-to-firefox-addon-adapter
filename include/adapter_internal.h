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
};

struct _AcreFxAdapter {
    WebKitWebContext *context;
    GHashTable *addons; /* addon_id -> AcreFxAddon */
    GPtrArray *loaded_paths;
    gpointer message_bus;
};

AcreFxManifest *acrefx_manifest_parse(const gchar *manifest_path);
void acrefx_manifest_free(AcreFxManifest *manifest);

#endif /* ADAPTER_INTERNAL_H */
