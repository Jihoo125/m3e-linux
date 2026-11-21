#!/usr/bin/env bash
set -euo pipefail
for file in /var/tmp/materialized-iso/config/common /var/tmp/materialized-iso/config/bootstrap /var/tmp/materialized-iso/config/chroot; do
    if [ -f "$file" ]; then
        echo "== $file =="
        grep -E 'LB_INIT|LB_BOOT|LB_BINARY|LB_MODE|LB_PARENT' "$file" || true
    fi
done
