#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
find "$ROOT/build/qml" -name qmldir -o -name libm3_qtplugin.so
ls -la "$ROOT/build/qml/qml/Materialized" || true
python3 -m pip install --user 'material-color-utilities==0.2.4'
