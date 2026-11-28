# JSON UI visual parity pass (Bedrock 1.26.65.1)

## Reference approach

The supplied Windows PE was inspected statically with `tools/bedrock_reference/inspect_ui_sprites.py`. `ui-sprite-client-evidence.json` records the SHA-256, version and property string/LEA candidates. It does not claim these candidates establish the original algorithms; some properties have no NUL-terminated matches in this build. Prior DDUI disassembly evidence remains in `dd-ui-client-evidence.json`.

Behavioral reference: Cinnabar's `dev/crates/json-ui` (https://github.com/cinnabar-mc/cinnabar/tree/dev/crates/json-ui), particularly `sprite.rs`, `sidecar.rs`, `layout.rs`, `expr.rs`, `emit.rs`. Consulted the public Bedrock JSON UI documentation and the previous implementation. Cinnabar is itself a reimplementation, not the vanilla client. Its repository is distributed under PolyForm Noncommercial 1.0.0; Perlin remains MIT. No Cinnabar source or assets are bundled; the changes are implemented in Perlin's C++ renderer. These source observations are compatibility hypotheses until native captures validate them.

## Changes to production code

- Treat JSON UI `clip_ratio` as the portion removed; accept bound `#clip_ratio` and `#clip-ratio`; default source-texel clipping and preserve the requested direction. Center clipping applies to both axes. Existing internal SceneWriter keep-fraction callers are unchanged.
- Parse scalar, symmetric pair, or four-value slice insets from control and texture metadata. Scale source insets separately in x/y using sidecar base size; preserve logical border size and proportionally reduce the destination caps when necessary. Handle meeting/overlapping source insets, UV regions and per-axis tiling including tile edges and nine-slice parts.
- Resolve contain versus cover using source aspect ratio. Keep fractional element positions until final device-pixel snapping. Default/zero UV size handling and tint alpha pass through to batches.
- Preserve full nesting rather than flattening after six scene groups. JSON UI `clips_children` inherits clip rectangles without imposing a private z-order group; regular clip children participate in stable global layer ordering. Existing interactive/scoped scene groups retain their local context. Resolve hit-test parent indices after paint order is known.
- Share primitive types and sidecar parser between the production renderer and standalone test harness. The optional bitmap-only switch is local to the visual harness; production font selection is unchanged.

## Actual rendering verification

Captured three pixel-only test scenes using Mesa llvmpipe 4.5 GLX on Xvfb. Independent expected images are constructed from explicit geometry/colors in `tools/ui_visual/generate_fixture.py`, not from actual captures. Total: 806,400 tested pixels, 0 mismatches. The diagnostic render includes labeled controls. External X11 desktop capture also matched the GL framebuffer diagnostic image exactly. Outputs are in `docs/rendering/visual-results/`.

| GUI scale | Framebuffer | Pixels | Different RGB pixels |
|---|---:|---:|---:|
| 1 | 320 x 180 | 57,600 | 0 |
| 2 | 640 x 360 | 230,400 | 0 |
| 3 | 960 x 540 | 518,400 | 0 |

## Boundaries of the result

Not a native Bedrock pixel-parity certification. Only the standalone renderer could run here; full CMake configure could not fetch the required perlin-api Git dependency because the container cannot resolve github.com. The Windows EXE cannot be launched (no installed Windows Bedrock package, resources, or compatible runtime). No real vanilla framebuffer or real server state was available. The screenshot was rendered with the Perlin UI vertex generation pipeline and GLX software backend, not the production Conduit/RenderDragon shader paths. No comparison with a Bedrock screenshot has been performed. Bilinear/grayscale material selection, complete font metrics and localization, full custom renderer dispatch, controller/hover/press/focus behavior, animation timing and the remaining DDUI/Ore UI rendering are not established as native-equivalent. Very large tiled regions remain subject to Perlin's 8192 tile guard. The existing DDUI state/form implementation is preserved but is not a native Ore UI port.

See `tools/ui_visual/README.md` to render external packs or compare genuine native screenshots.
