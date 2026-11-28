#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
output=${1:-/tmp/perlin-ui-visual-capture}
mkdir -p "$(dirname "$output")"
exec "${CXX:-g++}" -std=c++23 -O1 -g0 -ffunction-sections -fdata-sections -DPERLIN_UI_VISUAL_BITMAP_ONLY -Isrc \
  tools/ui_visual/capture.cpp tools/ui_visual/bitmap_only_ten.cpp \
  src/client/ui/{UiExpr,JsonUi,UiLayout,SceneEmit,SceneKit,Scene,HudData}.cpp \
  src/client/ui/draw/{Canvas,Color,SceneView,Style,Text}.cpp \
  src/client/systems/text/{McFont,FontProviderTables,FormatCodes}.cpp \
  src/util/{JsonValue,Resources,Paths}.cpp -Wl,--gc-sections -pthread -lX11 -l:libGL.so.1 -lpng -o "$output"
