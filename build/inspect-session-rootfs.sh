#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
CHROOT=/var/tmp/materialized-iso/chroot
printf '== Account ==\n'
chroot "$CHROOT" /usr/bin/id materialized
printf '\n== Dialout ==\n'
chroot "$CHROOT" /usr/bin/getent group dialout || true
printf '\n== Session files ==\n'
ls -l "$CHROOT/usr/share/materialized/qml/Materialized/qmldir" "$CHROOT/usr/share/materialized/qml/Materialized/M3Button.qml" "$CHROOT/usr/bin/materialized-session"
printf '\n== Launcher ==\n'
tail -55 "$CHROOT/usr/bin/materialized-session"
printf '\n== Profile ==\n'
cat "$CHROOT/home/materialized/.profile"
