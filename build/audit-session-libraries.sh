#!/usr/bin/env bash
set -euo pipefail
WORK=/var/tmp/materialized-iso
CHROOT="$WORK/chroot"
printf '== Runtime dependency audit ==\n'
MISSING=0
for binary in "$CHROOT"/usr/bin/mat* "$CHROOT"/usr/share/materialized/qml/Materialized/libm3plugin.so; do
    [ -f "$binary" ] || continue
    printf '%s\n' "${binary#$CHROOT}"
    if chroot "$CHROOT" /usr/bin/ldd "${binary#$CHROOT}" | grep 'not found'; then
        MISSING=1
    fi
done
exit "$MISSING"
