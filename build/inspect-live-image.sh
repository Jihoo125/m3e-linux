#!/usr/bin/env bash
set -euo pipefail
printf '== chroot_linux-image ==\n'
sed -n '1,140p' /usr/lib/live/build/lb_chroot_linux-image
printf '\n== helper loading ==\n'
grep -R -n 'LIVE_BUILD\|/usr/lib/live/build' /usr/lib/live/build/lb /usr/share/live/build/functions 2>/dev/null | head -30
printf '\n== mirror layout ==\n'
for path in dists/trixie/Contents-amd64.gz dists/trixie/main/Contents-amd64.gz dists/trixie/main/Contents-source.gz dists/trixie/Contents-source.gz; do
    printf '%s -> ' "$path"
    curl -s -o /dev/null -w '%{http_code}\n' "http://deb.debian.org/debian/$path"
done
