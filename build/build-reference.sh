#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="$ROOT/third_party/src/material-color-utilities"
if [ ! -f "$SOURCE/cpp/cam/hct.cc" ]; then
  echo "Run third_party/fetch.sh first" >&2
  exit 2
fi
mkdir -p "$ROOT/build/reference"
mapfile -t SOURCES < <(find "$SOURCE/cpp" -name '*.cc' ! -name '*_test.cc' | sort)
g++ -std=c++17 -O2 -I "$SOURCE" -include optional "$ROOT/tools/m3-reference/reference_dump.cpp" "${SOURCES[@]}" \
    $(pkg-config --libs absl_flat_hash_map absl_strings) \
    -o "$ROOT/build/reference/mcu-reference"
