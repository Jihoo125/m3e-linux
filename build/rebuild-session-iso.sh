#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
CHROOT=/var/tmp/materialized-iso/chroot
chroot "$CHROOT" /usr/sbin/usermod --append --groups dialout materialized
chroot "$CHROOT" /usr/bin/id materialized
mkdir -p "$CHROOT/etc/udev/rules.d"
printf 'KERNEL=="ttyS0", GROUP="dialout", MODE="0660"\n' > "$CHROOT/etc/udev/rules.d/60-materialized-serial.rules"
MAT_STAGING=/var/tmp/materialized-iso/session-root bash "$ROOT/platform/packages/stage-session.sh"
cp -a /var/tmp/materialized-iso/session-root/usr/bin/. "$CHROOT/usr/bin/"
cp -a /var/tmp/materialized-iso/session-root/usr/share/materialized/qml/Materialized/. "$CHROOT/usr/share/materialized/qml/Materialized/"
cp -a /var/tmp/materialized-iso/session-root/usr/share/applications/. "$CHROOT/usr/share/applications/"
cp -a /var/tmp/materialized-iso/session-root/usr/lib/systemd/user/. "$CHROOT/usr/lib/systemd/user/"
cp -a /var/tmp/materialized-iso/session-root/usr/share/backgrounds/. "$CHROOT/usr/share/backgrounds/"
mkdir -p "$CHROOT/etc/xdg/autostart"
rm -f "$CHROOT/etc/xdg/autostart/materialized-terminal.desktop" \
      "$CHROOT/etc/xdg/autostart/materialized-browser.desktop"
if [ -d /var/tmp/materialized-iso/session-root/etc/xdg/autostart ]; then
    cp -a /var/tmp/materialized-iso/session-root/etc/xdg/autostart/. "$CHROOT/etc/xdg/autostart/"
fi
cp /var/tmp/materialized-iso/session-root/etc/materialized/matcomp.toml "$CHROOT/etc/materialized/matcomp.toml"
bash "$ROOT/build/rebuild-diagnostic-iso.sh"
