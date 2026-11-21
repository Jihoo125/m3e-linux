#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
WORK=/var/tmp/materialized-iso
CHROOT="$WORK/chroot"
MAT_STAGING="$WORK/session-root" MAT_BUILD_ROOT="$ROOT/build" \
    bash "$ROOT/platform/packages/stage-session.sh"
for path in usr/bin usr/share/applications usr/share/materialized usr/share/backgrounds usr/lib/systemd/user etc/materialized MANIFEST.sha256; do
    if [ -e "$WORK/session-root/$path" ]; then
        cp -a "$WORK/session-root/$path" "$CHROOT/$(dirname "$path")/"
    fi
done
# libwlroots.so.10 is the compositor ABI used by this development build.
if [ -f "$WORK/session-root/usr/lib/x86_64-linux-gnu/libwlroots.so.10" ]; then
    install -Dm755 "$WORK/session-root/usr/lib/x86_64-linux-gnu/libwlroots.so.10" \
        "$CHROOT/usr/lib/x86_64-linux-gnu/libwlroots.so.10"
fi
# Execute only the rootfs preparation portion of the full build. It generates
# the current session hook and bootloader configuration without debootstrapping.
awk '/^mkdir -p "\$WORK\/live"$/{ exit } { print }' "$ROOT/platform/iso/build.sh" | \
    sed "s|^ROOT=.*|ROOT=\"$ROOT\"|" > "$WORK/prepare-current.sh"
# shellcheck disable=SC1090
source "$WORK/prepare-current.sh"
# Execute the exact chroot hook used by the corrected ISO builder.
/usr/sbin/chroot "$CHROOT" /bin/bash "$WORK/config/hooks/010-materialized-session.chroot"
printf '\n== Session account ==\n'
/usr/sbin/chroot "$CHROOT" /usr/bin/id materialized
printf '\n== Missing compositor/shell libraries ==\n'
for binary in /usr/bin/matshell /usr/bin/matcomp /usr/share/materialized/qml/Materialized/libm3plugin.so; do
    printf '%s\n' "$binary"
    /usr/sbin/chroot "$CHROOT" /bin/bash -c "LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu ldd '$binary' | grep 'not found' || true"
done
printf '\n== Final ISO ==\n'
bash "$ROOT/platform/iso/finalize-iso.sh"
