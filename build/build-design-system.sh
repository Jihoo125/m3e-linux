#!/usr/bin/env bash
# Build the Material design system and its visual QA application.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build/design-system}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
cmake -S "$ROOT/libs/libm3-qt" -B "$BUILD_DIR" -G Ninja \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DQT_QML_OUTPUT_DIRECTORY="$BUILD_DIR"
cmake --build "$BUILD_DIR" --parallel "${JOBS:-$(nproc)}"
