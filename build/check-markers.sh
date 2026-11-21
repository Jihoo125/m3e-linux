#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/iris8/Downloads/yes
for f in build/vm/boot-uefi.log build/vm/boot-bios.log; do
    printf '%s markers=%s ' "$f" "$(grep -a -c 'Materialized session started' "$f" || true)"
    grep -a -o 'Shell visible=1 exposed=1 geometry=[0-9]*x[0-9]*' "$f" | tail -1
done
