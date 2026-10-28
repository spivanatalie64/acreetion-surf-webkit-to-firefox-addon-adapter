#define _GNU_SOURCE
#include <glib.h>
#include "acrefxadapter.h"
#include "adapter_internal.h"

typedef struct {
    GHashTable *listeners; /* port_id -> callback */
} MessageBus;

MessageBus *acrefx_message_bus_new(void) {
    MessageBus *bus = g_new0(MessageBus, 1);
    bus->listeners = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    return bus;
}

void acrefx_message_bus_free(MessageBus *bus) {
    if (!bus) return;
    if (bus->listeners) g_hash_table_unref(bus->listeners);
    g_free(bus);
}
