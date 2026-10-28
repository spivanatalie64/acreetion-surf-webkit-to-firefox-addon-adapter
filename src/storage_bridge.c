#define _GNU_SOURCE
#include <glib.h>
#include <json-glib/json-glib.h>
#include "acrefxadapter.h"
#include "adapter_internal.h"

typedef struct {
    gchar *addon_id;
    gchar *storage_path;
    JsonObject *data;
} AcreFxStorage;

AcreFxStorage *acrefx_storage_new(const gchar *addon_id, const gchar *config_dir) {
    AcreFxStorage *storage = g_new0(AcreFxStorage, 1);
    storage->addon_id = g_strdup(addon_id);
    storage->storage_path = g_build_filename(config_dir, "addons", addon_id, "storage.json", NULL);
    storage->data = json_object_new();
    return storage;
}

void acrefx_storage_free(AcreFxStorage *storage) {
    if (!storage) return;
    g_free(storage->addon_id);
    g_free(storage->storage_path);
    if (storage->data) json_object_unref(storage->data);
    g_free(storage);
}
