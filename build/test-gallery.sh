#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# The generated module includes an explicit plugin, no patch needed.
python3 - "$ROOT/build/qml/qml/Materialized/qmldir" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
print("QML module:", p)
if p.exists():
    print(p.read_text(encoding='utf-8').splitlines()[:4])
else:
    raise SystemExit("QML module not built; run build/build-design-system.sh")
PY
cd "$ROOT"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml"
unset QML_IMPORT_TRACE
unset QT_DEBUG_PLUGINS
if [ "${1:-}" = "--screenshot" ]; then
    ./build/qml/bin/matgallery &
    PID=$!
    sleep 3
    mkdir -p build/screenshots
    import -window root build/screenshots/gallery.png || true
    kill "$PID" 2>/dev/null || true
else
    timeout "${GALLERY_TIMEOUT:-5s}" ./build/qml/bin/matgallery
    code=$?
    if [ "$code" = 124 ]; then exit 0; fi
    exit "$code"
fi
