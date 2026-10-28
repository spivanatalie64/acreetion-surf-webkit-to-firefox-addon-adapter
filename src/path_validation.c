#define _GNU_SOURCE
#include <string.h>
#include <glib.h>
#include <sys/stat.h>
#include <stdlib.h>

gboolean acrefx_is_safe_path(const gchar *base_path, const gchar *user_path) {
    if (!base_path || !user_path) return FALSE;
    
    /* Check for path traversal */
    if (strstr(user_path, "..") != NULL) {
        return FALSE;
    }
    
    /* Check for absolute paths outside addon dir */
    if (g_path_is_absolute(user_path)) {
        /* Only allow if it's within base_path */
        gchar *resolved_base = realpath(base_path, NULL);
        gchar *resolved_user = realpath(user_path, NULL);
        if (!resolved_base || !resolved_user) {
            g_free(resolved_base);
            g_free(resolved_user);
            return FALSE;
        }
        gboolean safe = g_str_has_prefix(resolved_user, resolved_base);
        g_free(resolved_base);
        g_free(resolved_user);
        return safe;
    }
    
    /* Build full path and resolve */
    gchar *full_path = g_build_filename(base_path, user_path, NULL);
    gchar *resolved_base = realpath(base_path, NULL);
    gchar *resolved_full = realpath(full_path, NULL);
    
    gboolean safe = FALSE;
    if (resolved_base && resolved_full) {
        safe = g_str_has_prefix(resolved_full, resolved_base);
    }
    
    g_free(full_path);
    g_free(resolved_base);
    g_free(resolved_full);
    
    return safe;
}
