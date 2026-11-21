#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
export XDG_RUNTIME_DIR="/tmp/materialized-runtime-$UID"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 0700 "$XDG_RUNTIME_DIR"
export WLR_RENDERER=pixman
export WAYLAND_DISPLAY=wayland-0
rm -f "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY.lock"
"$ROOT/build/matcomp/matcomp" --headless > build/compositor-client.log 2>&1 &
COMPOSITOR_PID=$!
cleanup() {
    kill "$COMPOSITOR_PID" "$GALLERY_PID" 2>/dev/null || true
    wait "$COMPOSITOR_PID" 2>/dev/null || true
}
GALLERY_PID=0
trap cleanup EXIT
for _ in $(seq 1 30); do
    if [ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]; then break; fi
    if ! kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
        cat build/compositor-client.log
        exit 1
    fi
    sleep .1
done
[ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ] || { echo "Compositor socket missing"; exit 1; }
export QT_QPA_PLATFORM=wayland
export QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml/qml"
export LIBGL_ALWAYS_SOFTWARE=1
"$ROOT/build/qml/bin/matgallery" > build/gallery-wayland.log 2>&1 &
GALLERY_PID=$!
sleep 4
kill -0 "$GALLERY_PID" 2>/dev/null || { cat build/gallery-wayland.log; exit 1; }
echo "Compositor and a Qt/Wayland client remained healthy for four seconds."
kill "$GALLERY_PID" 2>/dev/null || true
kill "$COMPOSITOR_PID" 2>/dev/null || true
wait "$GALLERY_PID" 2>/dev/null || true
wait "$COMPOSITOR_PID" 2>/dev/null || true
cat build/compositor-client.log
cat build/gallery-wayland.log
