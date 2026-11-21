#!/usr/bin/env bash
# Launch the component gallery under any Wayland compositor, or WSLg/XWayland.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build/design-system}"
export QML2_IMPORT_PATH="$BUILD_DIR${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland;xcb}"
export QT_QUICK_CONTROLS_STYLE=Basic
export QT_QUICK_BACKEND="${QT_QUICK_BACKEND:-software}"
export LIBGL_ALWAYS_SOFTWARE=1
if [ ! -x "$BUILD_DIR/matgallery" ]; then
    bash "$ROOT/build/build-design-system.sh"
fi
exec "$BUILD_DIR/matgallery" "$@"
