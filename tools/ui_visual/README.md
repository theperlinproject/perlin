# JSON UI visual regression harness

This Linux-only test tool renders the **production** Perlin JSON UI resolver, layout, scene emitter, text/canvas and batch generation into an actual X11/GLX window. Mesa OpenGL supplies the test rasterizer, not the production Conduit renderer. No game launch, Minecraft executable, proprietary textures, or third-party JSON UI implementation is required.

## Reproduce the synthetic regression

Requires GCC/Clang with C++23, X11 development headers, libGL.so.1 (Mesa GLX / llvmpipe works), libpng, Xvfb, xauth, and Python 3 with Pillow. The standalone build avoids FetchContent dependencies. The optional Ten/TTF font is disabled in this tool only; it uses Perlin's bitmap fallback or a provided `font/default8.png` sheet.

```sh
tools/ui_visual/verify.sh /tmp/perlin-ui-visual-results
```

The tool generates an original synthetic pack and separately constructs expected pixel images from explicitly authored rectangles and color patterns. It captures the screen at GUI scales 1, 2, and 3. The pixel-only fixture checks clipping direction/quantization, zero/full clips, texture sidecar scaling, tiling, contain/cover, UVs, fractional positions, nine nested clipping ancestors, cross-container layer interleaving, alpha blending, and a live binding. The labeled diagnostic image is for human inspection and is not a font-parity assertion. The strict comparator fails on any RGB channel difference or a dimension mismatch.

## Render a real resource-pack screen

```sh
tools/ui_visual/build.sh /tmp/perlin-ui-capture
xvfb-run -a -s '-screen 0 1280x720x24 +extension GLX' \
  /tmp/perlin-ui-capture namespace.screen 640 360 2 /tmp/perlin.png \
  /path/to/override-pack /path/to/vanilla-pack
```

The pack roots are highest priority first; each contains `ui/` and `textures/`. The screen name is a namespace/control key. `PERLIN_UI_VISUAL_DATA` can point to a JSON file with `bindings`, `collections` (arrays of binding objects) and `variables` objects to reproduce dynamic state. Capture reports unresolved controls, texture files, variables, and custom renderers. Custom engine renderers and native gameplay data are not emulated by the standalone tool. It does not capture DDUI/Ore UI. Only PNG textures and bitmap fonts are loaded by the harness; production pack support is broader.

To see the window externally, set `PERLIN_UI_VISUAL_HOLD_MS=5000` and use any X11 screen capture/VNC viewer on the same private display. `glReadPixels` saves the actual framebuffer, not a generated mockup. A desktop capture was independently obtained with `PIL.ImageGrab` on Xvfb; it matched the framebuffer exactly.

## Compare against Bedrock

Use a legitimately installed reference Bedrock build on Windows, apply the exact same resource-pack stack, and navigate to the same screen. Match language, viewport, GUI scale, input focus, game data, animation time and other state. Save lossless screenshots. The supplied fixture is a standalone namespace; to display it in native Bedrock, integrate its controls into an existing reachable screen in a test resource pack (e.g. a test-only form/HUD override) and supply equivalent binding data. It is not automatically a native screen override.

```sh
python3 tools/ui_visual/diff.py bedrock.png perlin.png --diff difference.png --report report.json
```

No resizing, image registration, masking, fuzzy threshold, or tolerance is used. Run the comparison on full-client screenshots to establish true native parity; synthetic oracles and the GLX harness alone cannot establish that result. The `capture` tool can help isolate mismatches in UI code before testing full-client shader, font, and input integration.
