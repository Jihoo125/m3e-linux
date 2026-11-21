#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${MAT_BUILD_ROOT:-$ROOT/build}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/materialized-$UID}"
mkdir -p "$XDG_RUNTIME_DIR/materialized/theme"
export QML2_IMPORT_PATH="$BUILD_ROOT/qml/qml"
export MAT_SCHEME_PATH="$XDG_RUNTIME_DIR/materialized/theme/scheme-dark.json"
export MAT_SCREENSHOT_PATH="$ROOT/build/screenshots/shell-dark.png"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_QUICK_BACKEND=software
export LIBGL_ALWAYS_SOFTWARE=1
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
"$BUILD_ROOT/matsessiond/matsessiond" --once --no-dbus --output "$XDG_RUNTIME_DIR/materialized/theme" --wallpaper "$ROOT/design/wallpaper/materialized-dawn.png" --mode dark
export MAT_WALLPAPER="$ROOT/design/wallpaper/materialized-dawn.png"
mkdir -p "$ROOT/build/screenshots"
"$BUILD_ROOT/matshell/matshell"
