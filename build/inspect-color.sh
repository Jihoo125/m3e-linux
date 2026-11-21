#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
for seed in FF6750A4 FF000000 FFFFFF00 FF123456; do
    echo "== $seed"
    for v in 2 5 6; do
        echo "variant $v"
        ./build/reference/mcu-reference "$seed" "$v" | grep -E '^(Primary|Secondary|Tertiary|SurfaceTint) '
        ./build/m3/m3-dump "$seed" light "$v" | grep -E '^(primary|secondary|tertiary|surfaceTint) '
    done
done
