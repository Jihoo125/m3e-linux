#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
cat "$ROOT/build/qml/qml/Materialized/qmldir" | head -8
ls -la "$ROOT/build/qml/qml/Materialized"
ldd "$ROOT/build/qml/qml/Materialized/libm3plugin.so" | head -12
cd "$ROOT"
export QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software QML2_IMPORT_PATH="$ROOT/build/qml" QML_IMPORT_TRACE=1 QT_DEBUG_PLUGINS=1 LIBGL_ALWAYS_SOFTWARE=1
if [ "${VERBOSE_QML:-}" = 1 ]; then
  timeout 3s ./build/qml/bin/matgallery > build/gallery-trace.log 2>&1 || true
else
  timeout 3s ./build/qml/bin/matgallery > build/gallery-trace.log 2>&1 || true
fi
python3 - "$ROOT/build/gallery-trace.log" <<'PY'
from pathlib import Path
import sys
for line in Path(sys.argv[1]).read_text(errors='replace').splitlines():
    if any(x in line.lower() for x in ['materialized', 'cannot load', 'not found', 'failed', 'library', 'singleton']):
        print(line)
PY
