#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="$ROOT/third_party/src/material-color-utilities"
mkdir -p "$ROOT/build/reference"
mapfile -t SOURCES < <(find "$SOURCE/cpp" -name '*.cc' ! -name '*_test.cc' | sort)
g++ -std=c++17 -O2 -I "$SOURCE" -include optional "$ROOT/tools/m3-reference/reference_temperature.cpp" "${SOURCES[@]}" \
    $(pkg-config --libs absl_flat_hash_map absl_strings) -o "$ROOT/build/reference/mcu-temp"
g++ -std=c++20 -O2 -I "$ROOT/libs/libm3/include" "$ROOT/libs/libm3/temperature_dump.cpp" "$ROOT/build/m3/libm3.a" -o "$ROOT/build/m3/temperature-dump"
diff -u <("$ROOT/build/reference/mcu-temp" FF6750A4) <("$ROOT/build/m3/temperature-dump" FF6750A4)
