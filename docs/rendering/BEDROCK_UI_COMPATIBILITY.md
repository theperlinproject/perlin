# Bedrock UI compatibility pass (1.26.65.1)

## Scope and reverse-engineering evidence

Reference: supplied Minecraft.Windows.exe, product/file version 1.26.65.1, protocol schema 2225 in `protocol/bedrock-1.26.60.json`. The SHA-256, image base, CodeView metadata, string locations, and instruction/function RVAs are preserved in `dd-ui-client-evidence.json`. These are results of static PE inspection and disassembly, not source code or a complete decompilation. No Microsoft executable or executable code is included in this archive.

- JSON UI: references to anchor, size, grid, binding, and rendering property names were located in the client binary. The layout corrections use documented semantics for these properties, rather than claiming every native layout algorithm has been recovered.
- DDUI: the custom form and message box code references the names `custom_form_data_{}` and `message_box_data_{}`, and constructs the `modalForm` property. Row and message fields were identified from string references and immediate values in disassembly. The path spelling and model conversion are compatibility hypotheses until confirmed against live packets.

Public references:
- https://learn.microsoft.com/en-us/minecraft/creator/reference/content/jsonuireference/examples/jsonuilist?view=minecraft-bedrock-stable
- https://learn.microsoft.com/en-us/minecraft/creator/documents/scripting/intro-to-ddui?view=minecraft-bedrock-stable
- https://wiki.bedrock.dev/json-ui/json-ui-documentation
- https://wiki.bedrock.dev/json-ui/dynamic-content-generation

## Implemented

- Shared JSON UI layout: parent percentage dimensions, own-axis `%x`/`%y` sizes and offsets, viewport-first sizing, default dimension fallback, stack offsets, shared fill allocation retained through sibling remeasurement, finite size limits, explicit grid positions, fill direction, sparse cell spacing, and prefix-sum grid placement.
- DDUI transport: replace the four no-op clientbound handlers with data store and screen lifecycle handlers; retain property/path revisions, reject stale and incompatible scalar writes, remove all properties on store removal, handle screen open/close and busy responses, invalidate state on reload/world changes, and send versioned serverbound data updates. Native overlays remain distinct from legacy ModalFormResponsePacket handling.
- DDUI model adapter: built-in custom-form and message-box snapshot parsing; native fallback for text, header, divider, spacer, toggle, slider, text input and dropdown, and conditional button support when the data contains a boolean `onClick` path. Changes to supported scalar fields are sent to the server. Missing/unsupported fields are not synthesized into arbitrary writes. Preserve focus for matching writable paths, update the visible model on data revisions, and separate close responses from legacy forms.
- Legacy fallback: dropdown options actually display, first-digit slider IDs work, and disabled/hidden controls are not interactive.

## Verification

Built and ran these standalone targets with GCC 14.2 and C++23:
- `ui_layout_selftest` -- passed (viewport, cross-axis dimensions, anchors, offsets, default, stack fill, sparse grids and vertical grid fill).
- `ddui_state_selftest` -- passed (store naming, generation/path counters, stale updates, literal/nested paths, type validation, replacement and store removal).
- `ui_library_selftest` -- passed (existing resource pack/namespace regression).

The layout-only test stubs TenFont because it tests geometry, not real glyphs. No complete Windows client build, GPU screenshot comparison, touch/gamepad test, or live Bedrock server/DDUI integration test was possible in this environment. The FormScreen and session integration are not certified by these isolated tests.

## Remaining work for 1:1 parity

This is a compatibility implementation, not a complete port of the original client. DDUI still needs the Ore UI composition/style/runtime (not a JSON UI skin), native callbacks/event contracts, styles, custom resource-pack DDUI screens, images, tooltips, horizontal message layouts, advanced bindings, and live protocol traces to verify paths, data IDs, and counter behavior. JSON UI requires further native behavioral comparison across screen factories, bindings, renderer types, sprite sampling, clipping, font metrics/wrapping, focus/navigation, animations, and GUI scaling. Use the same resource-pack stack, GUI scale, resolution, game state and language for pixel comparisons against the supplied client version.

## Follow-up visual regression pass

See `BEDROCK_VISUAL_PARITY.md` for the subsequent sprite, clipping, scene ordering and Xvfb rendering improvements. The standalone GLX harness was verified with synthetic pixel oracles; the full client and native Bedrock comparisons remain unverified.
