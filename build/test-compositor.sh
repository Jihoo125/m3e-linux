#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
cmake --build build/matcomp --parallel 4
rm -rf build/compositor-runtime
mkdir -p build/compositor-runtime
export XDG_RUNTIME_DIR="$ROOT/build/compositor-runtime"
export WLR_BACKENDS=headless
export WLR_RENDERER=pixman
export WAYLAND_DISPLAY=materialized-test
"$ROOT/build/matcomp/matcomp" --headless --self-test
