# Security & Compatibility Audit - Content Script Injection

## Approach
Using WebKitUserContentManager with WebKitUserScript/WebKitUserStyleSheet.

## Security Assessment

### Strengths
- **Sandboxing**: WebKitGTK provides its own sandboxing model. Content scripts run in the page context but we control injection timing.
- **No eval/Function constructor abuse**: We read files and inject as scripts via WebKit APIs, not via eval().
- **Path validation needed**: Currently using g_build_filename which is reasonable but needs validation.
- **API bridge isolation**: We prepend our API bridge before addon scripts.

### Concerns & Issues

#### Critical
1. **No path traversal protection** - JS/CSS file paths from manifest could contain `..`. Need to sanitize.
2. **No CSP handling** - Some pages have strict CSP that may block inline-style/script injection. WebKit's user content injection usually bypasses page CSP, but need to verify.
3. **All frames injection** - Using `WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES` is broad; Firefox has more granular control per frame.

#### High
4. **Match patterns ignored** - We parse content_scripts but don't filter by URL patterns. This means scripts run on ALL pages. Major issue vs Firefox behavior.
5. **run_at timing** - Hardcoded to `DOCUMENT_START`. Firefox allows `document_start`, `document_end`, `document_idle`. Need to respect manifest.
6. **No world isolation** - All scripts run in main world. Firefox uses "isolated world" by default for content scripts (different from page JS). This is a big compatibility issue.
7. **No permission checks** - Not validating if addon has required permissions for the sites.

#### Medium
8. **Memory leaks** - JsonParser objects need proper cleanup in all paths (we do mostly).
9. **No cleanup on reload** - When adapter reloads, old scripts remain in UserContentManager.
10. **File reading without size limits** - Large files could consume memory.

## Compatibility Assessment

### Works Well
- Basic content script injection for simple addons
- CSS injection
- Multiple scripts per content_scripts entry
- MV2/MV3 manifest parsing

### Poor Compatibility
- **Isolated worlds**: WebKitGTK doesn't have the same isolated world concept as Firefox/Chrome. This breaks addons that rely on it.
- **Match pattern matching**: Firefox uses sophisticated match patterns (`*://*.example.com/*`, `<all_urls>`, etc.). We need WebKitUserContentFilter or manual URL checking.
- **Messaging**: Content scripts need to communicate with background via `browser.runtime.sendMessage` - our bridge is minimal.
- **Storage API**: Not implemented.
- **Cross-origin**: Different behavior than Firefox.

## Recommendations

### Immediate fixes needed:
1. Add path traversal protection (`..` check, resolve realpath)
2. Implement match pattern matching (at minimum support basic patterns)
3. Respect `run_at` from manifest
4. Add URL filtering before injection
5. Handle script execution order correctly

### Long term:
1. Investigate WebKit's execution world options
2. Implement proper message passing
3. Add permission model
4. Consider using WebKitWebExtension for better control (like Firefox uses XUL/XPCOM historically)

## Verdict
**Not production safe yet.** Works for very simple proof-of-concept addons but has critical security and compatibility gaps. Needs hardening before any real use.
