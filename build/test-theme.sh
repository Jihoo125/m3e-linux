#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
cmake --build build/matsessiond --parallel 4
"$ROOT/build/matsessiond/matsessiond" --once --no-dbus --wallpaper "$ROOT/design/wallpaper/materialized-dawn.png" --mode dark --output "$ROOT/build/theme"
python3 - "$ROOT/build/theme/scheme-dark.json" <<'PY'
import json
from pathlib import Path
import sys
scheme = json.loads(Path(sys.argv[1]).read_text())
print('mode:', scheme['mode'])
print('source:', scheme['sourceColor'])
print('primary:', scheme['roles']['primary'])
print('surface:', scheme['roles']['surface'])
assert scheme['mode'] == 'dark'
assert scheme['sourceColor'] != '#6750A4'
assert scheme['roles']['primary'].startswith('#')
PY
export MAT_SCHEME_PATH="$ROOT/build/theme/scheme-dark.json"
exec "$ROOT/build/check-gallery-window.sh"
