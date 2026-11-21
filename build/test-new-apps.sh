#!/usr/bin/env bash
# Smoke-test the new apps as real Wayland clients under the headless compositor.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
export XDG_RUNTIME_DIR="/tmp/materialized-newapps-$UID"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 0700 "$XDG_RUNTIME_DIR"
export WLR_RENDERER=pixman
rm -f "$XDG_RUNTIME_DIR/wayland-0" "$XDG_RUNTIME_DIR/wayland-0.lock"
"$ROOT/build/matcomp/matcomp" --headless > "$ROOT/build/newapps-comp.log" 2>&1 &
COMP=$!
trap 'kill "$COMP" 2>/dev/null || true; wait "$COMP" 2>/dev/null || true' EXIT
for _ in $(seq 1 50); do [ -S "$XDG_RUNTIME_DIR/wayland-0" ] && break; sleep 0.1; done
[ -S "$XDG_RUNTIME_DIR/wayland-0" ] || { cat "$ROOT/build/newapps-comp.log"; exit 1; }
export WAYLAND_DISPLAY=wayland-0
export QT_QPA_PLATFORM=wayland
export QT_QUICK_BACKEND=software
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml/qml"
FAIL=0
for app in matterminal matview matcal matshot matcalc mattext matsysmon matfiles matsettings; do
    "$ROOT/build/$app/$app" > "$ROOT/build/newapps-$app.log" 2>&1 &
    PID=$!
    sleep 3
    if kill -0 "$PID" 2>/dev/null; then
        printf '%s: running as a Wayland client\n' "$app"
        kill "$PID" 2>/dev/null || true
        wait "$PID" 2>/dev/null || true
    else
        printf '%s: FAILED to stay alive\n' "$app" >&2
        cat "$ROOT/build/newapps-$app.log" >&2 || true
        FAIL=1
    fi
done
exit "$FAIL"
