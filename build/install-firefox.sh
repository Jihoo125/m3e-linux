#!/usr/bin/env bash
# Install Firefox ESR into the existing live rootfs (fast path; the full ISO
# builder also lists firefox-esr for clean rebuilds).
set -euo pipefail
CHROOT=/var/tmp/materialized-iso/chroot
mount --bind /proc "$CHROOT/proc"
mount --bind /sys "$CHROOT/sys"
mount --bind /dev "$CHROOT/dev"
mount --bind /dev/pts "$CHROOT/dev/pts"
trap 'umount -l "$CHROOT/dev/pts" "$CHROOT/dev" "$CHROOT/sys" "$CHROOT/proc" 2>/dev/null || true' EXIT
cp /etc/resolv.conf "$CHROOT/etc/resolv.conf"
chroot "$CHROOT" /usr/bin/env -i HOME=/root PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin DEBIAN_FRONTEND=noninteractive /bin/bash -c 'apt-get update && apt-get install -y --no-install-recommends firefox-esr'
chroot "$CHROOT" /usr/bin/firefox-esr --version
