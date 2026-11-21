#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
cmake --build build/qml -j4
mkdir -p build/screenshots
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml/qml"
export LIBGL_ALWAYS_SOFTWARE=1
export MAT_SCREENSHOT_PATH="$ROOT/build/screenshots/matgallery.png"
timeout 10s "$ROOT/build/qml/bin/matgallery"
echo "Screenshot: $ROOT/build/screenshots/matgallery.png"
