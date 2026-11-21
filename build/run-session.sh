#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${MAT_BUILD_ROOT:-$ROOT/build}"
RUNTIME="${XDG_RUNTIME_DIR:-/tmp/materialized-$UID}"
export XDG_RUNTIME_DIR="$RUNTIME"
mkdir -p "$RUNTIME/materialized/theme"
chmod 0700 "$RUNTIME"

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland}"
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$BUILD_ROOT/qml/qml${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
export MAT_SCHEME_PATH="$RUNTIME/materialized/theme/scheme-light.json"
export WLR_RENDERER="${WLR_RENDERER:-pixman}"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"

if ! command -v matcomp >/dev/null 2>&1 && [ ! -x "$BUILD_ROOT/matcomp/matcomp" ]; then
    echo "matcomp not built; run cmake --build build/matcomp" >&2
    exit 1
fi
COMP="$BUILD_ROOT/matcomp/matcomp"
[ -x "$COMP" ] || COMP="$(command -v matcomp)"
SESSION="$BUILD_ROOT/matsessiond/matsessiond"
SHELL="$BUILD_ROOT/matshell/matshell"
[ -x "$SESSION" ] || SESSION="$(command -v matsessiond)"
[ -x "$SHELL" ] || SHELL="$(command -v matshell)"

if [ -f "$ROOT/design/wallpaper/materialized-dawn.png" ]; then
    "$SESSION" --output "$RUNTIME/materialized/theme" --wallpaper "$ROOT/design/wallpaper/materialized-dawn.png" --mode "${MAT_MODE:-dark}" --no-dbus --once >/dev/null || true
fi

cleanup() { kill "${COMP_PID:-0}" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
"$COMP" ${MAT_HEADLESS:+--headless} &
COMP_PID=$!
for _ in $(seq 1 40); do
    [ -S "$RUNTIME/wayland-0" ] && break
    if ! kill -0 "$COMP_PID" 2>/dev/null; then
        echo "matcomp exited; see its output above" >&2
        exit 1
    fi
    sleep .1
done
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
if [ -z "${WAYLAND_DISPLAY:-}" ]; then
    for candidate in "$RUNTIME"/wayland-*; do
        if [ -S "$candidate" ]; then export WAYLAND_DISPLAY="$(basename "$candidate")"; break; fi
    done
fi
"$SESSION" --output "$RUNTIME/materialized/theme" &
SESSION_PID=$!
trap 'kill "$SESSION_PID" "$COMP_PID" 2>/dev/null || true' EXIT INT TERM
exec "$SHELL" "$@"
