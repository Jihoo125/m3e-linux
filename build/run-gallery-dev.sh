#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=Basic
export QML2_IMPORT_PATH="$ROOT/build/qml/qml"
export LIBGL_ALWAYS_SOFTWARE=1
exec "$ROOT/build/qml/bin/matgallery"
