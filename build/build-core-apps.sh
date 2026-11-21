#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUILD_ROOT="${MAT_BUILD_ROOT:-$ROOT/build}"
for app in matcalc mattext matsysmon matfiles matsettings matterminal matview matshot matcal; do
    printf '\n==> %s\n' "$app"
    cmake -S "apps/$app" -B "$BUILD_ROOT/$app" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMAT_ENABLE_TESTS=ON
    cmake --build "$BUILD_ROOT/$app" --parallel "${JOBS:-4}"
done
ctest --test-dir "$BUILD_ROOT/matcalc" --output-on-failure
ctest --test-dir "$BUILD_ROOT/mattext" --output-on-failure
ctest --test-dir "$BUILD_ROOT/matterminal" --output-on-failure
ctest --test-dir "$BUILD_ROOT/matcal" --output-on-failure
