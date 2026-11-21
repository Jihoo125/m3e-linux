#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$ROOT/build/qml/qml/Materialized/qmldir" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
p.write_text(p.read_text().replace('plugin m3plugin', 'optional plugin m3plugin'), encoding='utf-8')
PY
cd "$ROOT"
export QT_QPA_PLATFORM=xcb
export QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml"
export LIBGL_ALWAYS_SOFTWARE=1
export QT_DEBUG_PLUGINS=1
if [ -n "${VERBOSE_QML:-}" ]; then export QML_IMPORT_TRACE=1; fi
if [ "${1:-}" = "--screenshot" ]; then
    ./build/qml/bin/matgallery > build/gallery.log 2>&1 &
    PID=$!
    trap 'kill "$PID" 2>/dev/null || true' EXIT
    sleep 3
    mkdir -p build/screenshots
    import -window root build/screenshots/gallery.png
    kill "$PID" 2>/dev/null || true
else
    timeout 5s ./build/qml/bin/matgallery 2>&1 | tee build/gallery.log
    code="${PIPESTATUS[0]}"
    cat build/gallery.log
    [ "$code" = 0 ] || [ "$code" = 124 ]
fi
