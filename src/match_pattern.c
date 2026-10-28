#define _GNU_SOURCE
#include <string.h>
#include <glib.h>
#include <regex.h>
#include "acrefxadapter.h"

typedef struct {
    gchar *scheme;
    gchar *host;
    gchar *path;
} ParsedPattern;

static void free_parsed_pattern(ParsedPattern *p) {
    if (!p) return;
    g_free(p->scheme);
    g_free(p->host);
    g_free(p->path);
    g_free(p);
}

static ParsedPattern *parse_match_pattern(const gchar *pattern) {
    if (!pattern) return NULL;
    
    if (g_strcmp0(pattern, "<all_urls>") == 0) {
        ParsedPattern *p = g_new0(ParsedPattern, 1);
        p->scheme = g_strdup("*");
        p->host = g_strdup("*");
        p->path = g_strdup("/*");
        return p;
    }
    
    /* Find :// */
    gchar *scheme_end = strstr(pattern, "://");
    if (!scheme_end) return NULL;
    
    ParsedPattern *p = g_new0(ParsedPattern, 1);
    p->scheme = g_strndup(pattern, scheme_end - pattern);
    
    const gchar *rest = scheme_end + 3;
    /* Find first / to separate host and path */
    gchar *path_start = strchr(rest, '/');
    if (!path_start) {
        /* Firefox requires a path, even if just / */
        free_parsed_pattern(p);
        return NULL;
    }
    
    p->host = g_strndup(rest, path_start - rest);
    p->path = g_strdup(path_start);
    
    return p;
}

static gboolean match_scheme(const gchar *pattern_scheme, const gchar *url) {
    if (!pattern_scheme || !url) return FALSE;
    
    if (g_strcmp0(pattern_scheme, "*") == 0) {
        /* * matches only http and https */
        return g_str_has_prefix(url, "http://") || g_str_has_prefix(url, "https://");
    }
    
    gchar *scheme_end = strstr(url, "://");
    if (!scheme_end) return FALSE;
    
    gsize url_scheme_len = scheme_end - url;
    if (url_scheme_len != strlen(pattern_scheme)) return FALSE;
    
    return strncmp(url, pattern_scheme, url_scheme_len) == 0;
}

static gboolean match_host(const gchar *pattern_host, const gchar *url) {
    if (!pattern_host || !url) return FALSE;
    
    gchar *scheme_end = strstr(url, "://");
    if (!scheme_end) return FALSE;
    const gchar *host_start = scheme_end + 3;
    
    gchar *path_start = strchr(host_start, '/');
    gsize host_len = path_start ? (path_start - host_start) : strlen(host_start);
    
    gchar *url_host = g_strndup(host_start, host_len);
    
    gboolean result = FALSE;
    
    if (g_strcmp0(pattern_host, "*") == 0) {
        result = TRUE;
    } else if (g_str_has_prefix(pattern_host, "*.")) {
        /* Match subdomains */
        const gchar *pattern_domain = pattern_host + 2;
        if (g_strcmp0(url_host, pattern_domain) == 0) {
            /* *.example.com does NOT match example.com itself */
            result = FALSE;
        } else if (g_str_has_suffix(url_host, pattern_domain)) {
            /* Check it's a subdomain (preceded by .) */
            gsize suffix_len = strlen(pattern_domain);
            if (host_len > suffix_len && url_host[host_len - suffix_len - 1] == '.') {
                result = TRUE;
            }
        }
    } else {
        /* Exact match */
        result = (g_strcmp0(url_host, pattern_host) == 0);
    }
    
    g_free(url_host);
    return result;
}

static gboolean match_path(const gchar *pattern_path, const gchar *url) {
    if (!pattern_path || !url) return FALSE;
    
    gchar *scheme_end = strstr(url, "://");
    if (!scheme_end) return FALSE;
    const gchar *path_start = strchr(scheme_end + 3, '/');
    if (!path_start) return FALSE;
    
    /* Firefox path matching: * matches any string, but not across / in some implementations
     * Actually * matches zero or more characters */
    const gchar *p = pattern_path;
    const gchar *u = path_start;
    
    while (*p && *u) {
        if (*p == '*') {
            p++;
            if (*p == '\0') {
                /* * at end matches rest */
                return TRUE;
            }
            /* Find next char after * in pattern */
            gchar next = *p;
            /* Skip through url until we find next char or end */
            while (*u && *u != next) {
                u++;
            }
            if (*u == '\0') return (*p == '\0');
            continue;
        }
        if (*p == *u) {
            p++;
            u++;
        } else {
            return FALSE;
        }
    }
    
    /* Handle remaining wildcards */
    while (*p == '*') p++;
    
    return (*p == '\0' && *u == '\0');
}

gboolean acrefx_match_pattern_matches(const gchar *pattern, const gchar *url) {
    if (!pattern || !url) return FALSE;
    
    ParsedPattern *p = parse_match_pattern(pattern);
    if (!p) return FALSE;
    
    gboolean result = match_scheme(p->scheme, url) && 
                      match_host(p->host, url) && 
                      match_path(p->path, url);
    
    free_parsed_pattern(p);
    return result;
}

gboolean acrefx_should_inject_script(JsonObject *cs_obj, const gchar *url) {
    if (!cs_obj || !url) return FALSE;
    
    if (!json_object_has_member(cs_obj, "matches")) {
        return FALSE;
    }
    
    /* Check exclude_matches first */
    if (json_object_has_member(cs_obj, "exclude_matches")) {
        JsonArray *exclude = json_object_get_array_member(cs_obj, "exclude_matches");
        if (exclude) {
            for (guint i = 0; i < json_array_get_length(exclude); i++) {
                JsonNode *node = json_array_get_element(exclude, i);
                if (JSON_NODE_HOLDS_VALUE(node)) {
                    const gchar *pattern = json_array_get_string_element(exclude, i);
                    if (pattern && acrefx_match_pattern_matches(pattern, url)) {
                        return FALSE;
                    }
                }
            }
        }
    }
    
    /* Check matches */
    JsonArray *matches = json_object_get_array_member(cs_obj, "matches");
    if (!matches) return FALSE;
    
    for (guint i = 0; i < json_array_get_length(matches); i++) {
        JsonNode *node = json_array_get_element(matches, i);
        if (JSON_NODE_HOLDS_VALUE(node)) {
            const gchar *pattern = json_array_get_string_element(matches, i);
            if (pattern && acrefx_match_pattern_matches(pattern, url)) {
                return TRUE;
            }
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
