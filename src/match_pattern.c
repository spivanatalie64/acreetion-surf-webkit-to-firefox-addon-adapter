#define _GNU_SOURCE
#include <string.h>
#include <glib.h>
#include <regex.h>
#include "acrefxadapter.h"

/* Convert Firefox match pattern to regex
 * Pattern format: <scheme>://<host><path> where scheme/host/path can have wildcards
 * * matches zero or more characters, ** not supported in simple form
 * <all_urls> matches all http, https, ftp, file URLs */
static gchar *pattern_to_regex(const gchar *pattern) {
    if (!pattern) return NULL;
    
    if (g_strcmp0(pattern, "<all_urls>") == 0) {
        return g_strdup("^(https?|ftp|file)://.*$");
    }
    
    GString *regex = g_string_new("^");
    
    const gchar *p = pattern;
    while (*p) {
        if (*p == '*') {
            /* * matches zero or more characters except maybe / in some cases, 
             * but Firefox is more complex. Use non-greedy-like simple match */
            g_string_append(regex, ".*");
        } else if (*p == '.') {
            g_string_append(regex, "\\.");
        } else if (*p == '?') {
            g_string_append(regex, ".");
        } else if (*p == '/' || *p == ':' || *p == '-' || *p == '_' || 
                   (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || 
                   (*p >= '0' && *p <= '9')) {
            g_string_append_c(regex, *p);
        } else {
            g_string_append_printf(regex, "\\%c", *p);
        }
        p++;
    }
    g_string_append(regex, "$");
    
    gchar *result = regex->str;
    g_string_free(regex, FALSE);
    return result;
}

gboolean acrefx_match_pattern_matches(const gchar *pattern, const gchar *url) {
    if (!pattern || !url) return FALSE;
    
    if (g_strcmp0(pattern, "<all_urls>") == 0) {
        /* Firefox allows <all_urls> for http, https, ws, wss, file, ftp */
        if (g_str_has_prefix(url, "http://") || g_str_has_prefix(url, "https://") ||
            g_str_has_prefix(url, "file://") || g_str_has_prefix(url, "ftp://")) {
            return TRUE;
        }
        return FALSE;
    }
    
    gchar *regex_str = pattern_to_regex(pattern);
    if (!regex_str) return FALSE;
    
    regex_t re;
    int ret = regcomp(&re, regex_str, REG_EXTENDED | REG_NOSUB | REG_ICASE);
    g_free(regex_str);
    
    if (ret != 0) {
        return FALSE;
    }
    
    ret = regexec(&re, url, 0, NULL, 0);
    regfree(&re);
    
    return (ret == 0);
}

gboolean acrefx_should_inject_script(JsonObject *cs_obj, const gchar *url) {
    if (!cs_obj || !url) return FALSE;
    
    /* If matches is not specified, Firefox doesn't inject */
    if (!json_object_has_member(cs_obj, "matches")) {
        return FALSE;
    }
    
    /* Check exclude_matches first - if it matches exclude, don't inject */
    if (json_object_has_member(cs_obj, "exclude_matches")) {
        JsonArray *exclude = json_object_get_array_member(cs_obj, "exclude_matches");
        if (exclude) {
            for (guint i = 0; i < json_array_get_length(exclude); i++) {
                const gchar *pattern = json_array_get_string_element(exclude, i);
                if (pattern && acrefx_match_pattern_matches(pattern, url)) {
                    return FALSE;
                }
            }
        }
    }
    
    /* Check include_globs if present */
    if (json_object_has_member(cs_obj, "include_globs")) {
        JsonArray *globs = json_object_get_array_member(cs_obj, "include_globs");
        if (globs && json_array_get_length(globs) > 0) {
            gboolean matched_glob = FALSE;
            for (guint i = 0; i < json_array_get_length(globs); i++) {
                const gchar *glob = json_array_get_string_element(globs, i);
                /* Simple glob match - for basic cases */
                if (glob) {
                    gchar *regex_glob = g_strdup(glob);
                    /* Very basic conversion */
                    for (gchar *c = regex_glob; *c; c++) {
                        if (*c == '*') *c = 'X'; /* placeholder */
                    }
                    /* Fall through to matches check for now */
                }
            }
        }
    }
    
    /* Check matches */
    JsonArray *matches = json_object_get_array_member(cs_obj, "matches");
    if (!matches) return FALSE;
    
    for (guint i = 0; i < json_array_get_length(matches); i++) {
        const gchar *pattern = json_array_get_string_element(matches, i);
        if (pattern && acrefx_match_pattern_matches(pattern, url)) {
            return TRUE;
        }
    }
    
    return FALSE;
}

WebKitUserScriptInjectionTime acrefx_get_injection_time(const gchar *run_at) {
    if (!run_at) {
        return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START;
    }
    
    if (g_strcmp0(run_at, "document_end") == 0) {
        return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;
    } else if (g_strcmp0(run_at, "document_idle") == 0) {
        return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_IDLE;
    }
    
    return WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START;
}
