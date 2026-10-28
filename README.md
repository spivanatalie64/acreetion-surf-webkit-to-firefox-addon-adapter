# acreetion-surf-webkit-to-firefox-addon-adapter

Adapter layer to run Firefox WebExtensions (WebExtensions API) inside [acreetion-surf](https://github.com/spivanatalie64/acreetion-surf), which uses WebKitGTK.

## Goal
Provide a compatibility shim between the `browser.*` WebExtension APIs expected by Firefox addons and WebKitGTK's extension/content-script mechanisms (WebKitWebExtension, UserContentManager, messaging).

## Approach
- Parse `manifest.json` (v2/v3 subset as needed)
- Inject content scripts via `WebKitUserContentManager`
- Bridge background scripts / popup / options via a custom message bus
- Map `browser.tabs`, `browser.runtime`, `browser.storage`, etc to WebKit/WebKitGTK equivalents
- Integrate with acreetion-surf's C/Rust shell

## Status
Scaffolded. Work in progress.
