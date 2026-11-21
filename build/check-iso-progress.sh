#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
printf '== ISO candidates ==\n'
find /var/tmp/materialized-iso "$ROOT/build/iso" -maxdepth 2 -name '*.iso' -printf '%p %s bytes\n' 2>/dev/null || true
printf '\n== Live session files ==\n'
if [ -d /var/tmp/materialized-iso/chroot/usr/bin ]; then
    find /var/tmp/materialized-iso/chroot/usr/bin -maxdepth 1 -name 'mat*' -printf '%f\n' | sort
fi
printf '\n== Package configuration errors (recent live-build log) ==\n'
if [ -f /var/tmp/materialized-iso/log ]; then
    tail -30 /var/tmp/materialized-iso/log
fi
printf '\n== Running build processes ==\n'
ps -eo pid,etime,args | grep -E 'lb build|apt-get|mksquashfs|xorrisofs' | grep -v grep || true
