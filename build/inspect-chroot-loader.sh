#!/usr/bin/env bash
set -euo pipefail
WORK=/var/tmp/materialized-iso
CHROOT="$WORK/chroot"
printf '== Dynamic loader ==\n'
ls -l "$CHROOT/lib/x86_64-linux-gnu/libc.so.6" "$CHROOT/usr/lib/x86_64-linux-gnu/libc.so.6" "$CHROOT/lib64/ld-linux-x86-64.so.2" || true
printf '\n== chroot through the loader ==\n'
"$CHROOT/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2" --library-path "$CHROOT/usr/lib/x86_64-linux-gnu" "$CHROOT/usr/bin/chroot" "$CHROOT" /bin/bash -c 'echo rootfs-ok; id' 2>&1 || true
printf '\n== chroot shell only ==\n'
/usr/sbin/chroot "$CHROOT" /bin/bash -c 'echo rootfs-ok; id' 2>&1 || true
