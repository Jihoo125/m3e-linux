#!/usr/bin/env bash
set -euo pipefail
if [ "$(id -u)" -ne 0 ]; then exec sudo -E bash "$0" "$@"; fi
# WSL2 drift also breaks incremental builds: files on /mnt/c retain a future
# timestamp, so Ninja regenerates CMake forever. Synchronise system time first.
"$(dirname "$0")/fix-wsl-clock.sh"
echo "Normalising mtimes of checked-in source files (contents are unchanged)."
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
find "$ROOT" \( -path "$ROOT/build" -o -path "$ROOT/third_party" -o -path "$ROOT/.git" -o -name '*.pyc' \) -prune -o -type f -exec touch -m -d '1 minute ago' -- {} +
echo "Source mtimes normalised. Re-run cmake/build now."
