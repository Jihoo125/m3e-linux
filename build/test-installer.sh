#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUILD="${BUILD_DIR:-$ROOT/build/installer-clean}"
cmake -S installer/matinstall-core -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD" --parallel 4
ctest --test-dir "$BUILD" --output-on-failure
