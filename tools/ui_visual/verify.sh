#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(realpath -m "${1:-/tmp/perlin-ui-visual-results}")
mkdir -p "$out"
pack="$out/pack"
python3 tools/ui_visual/generate_fixture.py "$pack" --expected "$out"
tools/ui_visual/build.sh "$out/capture"
export PERLIN_UI_VISUAL_DATA="$pack/state.json"
export LIBGL_ALWAYS_SOFTWARE=1
for scale in 1 2 3; do
  xvfb-run -a -s '-screen 0 960x540x24 +extension GLX' "$out/capture" parity.pixels 320 180 "$scale" "$out/actual-${scale}x.png" "$pack" | tee "$out/render-${scale}x.log"
  python3 tools/ui_visual/diff.py "$out/expected-${scale}x.png" "$out/actual-${scale}x.png" --diff "$out/diff-${scale}x.png" --report "$out/report-${scale}x.json"
done
xvfb-run -a -s '-screen 0 960x540x24 +extension GLX' "$out/capture" parity.diagnostic 320 180 3 "$out/diagnostic.png" "$pack" | tee "$out/render-diagnostic.log"
