#!/usr/bin/env bash
set -euo pipefail
if [ -f /var/tmp/materialized-iso/chroot/etc/apt/sources.list ]; then
    printf '== chroot apt sources ==\n'
    cat /var/tmp/materialized-iso/chroot/etc/apt/sources.list
fi
printf '\n== live-build security defaults ==\n'
grep -n -C 4 'PARENT_MIRROR_CHROOT_SECURITY\|PARENT_MIRROR_CHROOT=' /usr/share/live/build/functions/defaults.sh | head -80
printf '\n== script paths ==\n'
ls -ld /var/tmp/materialized-iso/local/live-build/scripts /var/tmp/materialized-iso/local/live-build/scripts/build 2>/dev/null || true
