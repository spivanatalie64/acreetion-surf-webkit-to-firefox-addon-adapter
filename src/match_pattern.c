#define _GNU_SOURCE
#include <string.h>
#include <glib.h>
#include "acrefxadapter.h"

/* Simple match pattern matching for Firefox WebExtension style patterns
 * Supports: *://*.example.com/*, *://example.com/*, http://*/*, <all_urls> */
gboolean acrefx_match_pattern_matches(const gchar *pattern, const gchar *url) {
    if (!pattern || !url) return FALSE;
    
    /* <all_urls> matches everything */
    if (g_strcmp0(pattern, "<all_urls>") == 0) {
        return TRUE;
    }
    
    /* Very basic implementation - needs improvement */
    /* For now, do simple string contains check as fallback */
    /* TODO: Implement full pattern matching per MDN spec */
    
    gchar *scheme_end = strstr(url, "://");
    if (!scheme_end) return FALSE;
    
    const gchar *scheme = url;
    const gchar *host_start = scheme_end + 3;
    gchar *path_start = strchr(host_start, '/');
    if (!path_start) return FALSE;
    
    /* Basic wildcard support */
    if (g_str_has_prefix(pattern, "*://")) {
        /* Match any scheme */
        const gchar *pattern_rest = pattern + 5;
        if (g_str_has_prefix(pattern_rest, "*.")) {
            /* Match subdomains */
            const gchar *domain = pattern_rest + 2;
            gchar *slash = strchr(domain, '/');
            if (slash) {
                gchar *domain_part = g_strndup(domain, slash - domain);
                if (strstr(host_start, domain_part)) {
                    g_free(domain_part);
                    return TRUE;
                }
                g_free(domain_part);
            }
        } else if (g_strcmp0(pattern_rest, "*/*") == 0) {
            return TRUE;
        }
    } else if (g_str_has_prefix(pattern, "http://") || g_str_has_prefix(pattern, "https://")) {
        /* Specific scheme */
        if (g_str_has_prefix(url, pattern)) {
            /* Very basic - would need full glob matching */
            return TRUE;
        }
    }
    
    return FALSE;
}

gboolean acrefx_should_inject_script(JsonObject *cs_obj, const gchar *url) {
    if (!cs_obj || !url) return FALSE;
    
    /* Check exclude matches first */
    if (json_object_has_member(cs_obj, "exclude_matches")) {
        JsonArray *exclude = json_object_get_array_member(cs_obj, "exclude_matches");
        for (guint i = 0; i < json_array_get_length(exclude); i++) {
            const gchar *pattern = json_array_get_string_element(exclude, i);
            if (acrefx_match_pattern_matches(pattern, url)) {
                return FALSE;
            }
        }
    }
    
    /* Check matches */
    if (json_object_has_member(cs_obj, "matches")) {
        JsonArray *matches = json_object_get_array_member(cs_obj, "matches");
        for (guint i = 0; i < json_array_get_length(matches); i++) {
            const gchar *pattern = json_array_get_string_element(matches, i);
            if (acrefx_match_pattern_matches(pattern, url)) {
                return TRUE;
            }
        }
        return FALSE;
    }
    
    /* If no matches specified, don't inject (Firefox behavior) */
    return FALSE;
}

WebKitUserScriptInjectionTime acrefx_get_injection_time(const gchar *run_at) {
    if (!run_at) {
        return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START;
    }
    
    if (g_strcmp0(run_at, "document_end") == 0) {
        return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;
    } else if (g_strcmp0(run_at, "document_idle") == 0) {
        return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END; /* Approximate */
    }
    
    return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START;
}
