#!/usr/bin/env bash
set -euo pipefail
if [ -d /var/tmp/materialized-iso/chroot ]; then
    printf '== chroot package manager ==\n'
    if command -v lsof >/dev/null; then
        lsof /var/tmp/materialized-iso/chroot/var/lib/dpkg/lock* 2>/dev/null || true
    fi
    tail -50 /var/tmp/materialized-iso/chroot/var/log/dpkg.log 2>/dev/null || true
    printf '\n== apt errors ==\n'
    grep -R -i 'unable\|error\|failed\|not found\|no installation candidate' /var/tmp/materialized-iso/chroot/var/log/apt 2>/dev/null | tail -20 || true
    printf '\n== staged session binaries ==\n'
    find /var/tmp/materialized-iso/chroot/usr/bin -maxdepth 1 -name 'mat*' -printf '%f\n' 2>/dev/null | sort || true
fi
