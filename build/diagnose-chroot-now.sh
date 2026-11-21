#!/usr/bin/env bash
set -euo pipefail
CHROOT=/var/tmp/materialized-iso/chroot
printf '== Shell and loader ==\n'
ls -l "$CHROOT/bin/bash" "$CHROOT/lib64/ld-linux-x86-64.so.2" "$CHROOT/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2" "$CHROOT/usr/bin/apt-get" "$CHROOT/usr/bin/apt" || true
printf '\n== chroot defaults ==\n'
chroot "$CHROOT" /bin/bash --version || true
printf '\n== explicit environment ==\n'
chroot "$CHROOT" /usr/bin/env -i HOME=/root PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin /bin/bash --version || true
printf '\n== explicit loader ==\n'
chroot "$CHROOT" /lib64/ld-linux-x86-64.so.2 --library-path /usr/lib/x86_64-linux-gnu /bin/bash --version || true
printf '\n== chroot mounts ==\n'
mount | grep "$CHROOT" || true
