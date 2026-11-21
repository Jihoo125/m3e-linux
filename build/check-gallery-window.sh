#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
cmake --build build/qml -j4
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml/qml"
export LIBGL_ALWAYS_SOFTWARE=1
export MAT_SCREENSHOT_PATH="$ROOT/build/screenshots/matgallery.png"
mkdir -p build/screenshots
if [ ! -f "$MAT_SCREENSHOT_PATH" ]; then
  timeout 10s ./build/qml/bin/matgallery > build/logs/gallery.log 2>&1
else
  rm "$MAT_SCREENSHOT_PATH"
  timeout 10s ./build/qml/bin/matgallery > build/logs/gallery.log 2>&1
fi
cat build/logs/gallery.log
test -s "$MAT_SCREENSHOT_PATH"
file "$MAT_SCREENSHOT_PATH"
