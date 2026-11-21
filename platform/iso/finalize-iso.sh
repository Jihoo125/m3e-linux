#!/usr/bin/env bash
# Assemble a GRUB 2 UEFI/BIOS live ISO from the live-build chroot.
# This replaces legacy live-build image finalization, which is BIOS-only on the
# Ubuntu 22.04 development host. No host live-build files are modified.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${MAT_ISO_WORK:-/var/tmp/materialized-iso}"
OUT="${MAT_ISO_OUTPUT:-$ROOT/build/iso/materialized-0.2.1-amd64.iso}"
CHROOT="$WORK/chroot"
BINARY="$WORK/binary"

for tool in grub-mkimage grub-mkstandalone mkfs.vfat mmd mcopy xorriso mksquashfs; do
    command -v "$tool" >/dev/null 2>&1 || { echo "Missing ISO tool: $tool" >&2; exit 1; }
done
[ -x "$CHROOT/usr/bin/matshell" ] || { echo "Missing session in $CHROOT" >&2; exit 1; }
# Fail before creating a large ISO when the staged binaries cannot start.
for binary in "$CHROOT"/usr/bin/mat* "$CHROOT"/usr/share/materialized/qml/Materialized/libm3plugin.so; do
    [ -f "$binary" ] || continue
    if chroot "$CHROOT" /usr/bin/ldd "${binary#$CHROOT}" | grep -q 'not found'; then
        chroot "$CHROOT" /usr/bin/ldd "${binary#$CHROOT}" | grep 'not found'
        exit 1
    fi
done
[ -f /usr/lib/ISOLINUX/isohdpfx.bin ] || { echo "Install syslinux-common" >&2; exit 1; }
mkdir -p "$(dirname "$OUT")"

if [ "${MAT_REUSE_SQUASHFS:-0}" = 1 ] && [ -s "$CHROOT/binary/live/filesystem.squashfs" ]; then
    mkdir -p "$WORK"
    cp "$CHROOT/binary/live/filesystem.squashfs" "$WORK/filesystem.squashfs"
fi
rm -rf "$CHROOT/binary" "$CHROOT/binary.hybrid.iso" "$CHROOT/binary.sh"
rm -rf "$BINARY"
mkdir -p "$BINARY/live" "$BINARY/boot/grub/i386-pc" "$BINARY/EFI/BOOT"
SQUASHFS="$BINARY/live/filesystem.squashfs"
if [ "${MAT_REUSE_SQUASHFS:-0}" = 1 ] && [ -s "$WORK/filesystem.squashfs" ]; then
    mv "$WORK/filesystem.squashfs" "$SQUASHFS"
else
    mksquashfs "$CHROOT" "$SQUASHFS" -no-progress -comp xz -noappend
fi

KERNEL="$(find "$CHROOT/boot" -maxdepth 1 -type f -name 'vmlinuz-*-amd64' | sort | head -1)"
INITRD="$(find "$CHROOT/boot" -maxdepth 1 -type f -name 'initrd.img-*-amd64' | sort | head -1)"
[ -n "$KERNEL" ] && [ -n "$INITRD" ] || { echo "Kernel or initramfs missing" >&2; exit 1; }
cp "$KERNEL" "$BINARY/live/vmlinuz"
cp "$INITRD" "$BINARY/live/initrd.img"
if [ -f "$CHROOT/var/lib/dpkg/status" ]; then
    python3 - "$CHROOT/var/lib/dpkg/status" "$BINARY/live/filesystem.packages" <<'PY'
from pathlib import Path
import sys
packages = []
for stanza in Path(sys.argv[1]).read_text().split('\n\n'):
    fields = {}
    for line in stanza.splitlines():
        if ': ' in line:
            key, value = line.split(': ', 1)
            fields[key] = value
    if fields.get('Status') == 'install ok installed' and 'Package' in fields:
        packages.append(f"{fields['Package']}={fields.get('Version', '')}")
Path(sys.argv[2]).write_text('\n'.join(sorted(packages)) + '\n')
PY
fi
cp "$BINARY/live/filesystem.packages" "$OUT.packages"

cat > "$BINARY/boot/grub/grub.cfg" <<'EOF'
set default=0
set timeout=3
set gfxpayload=keep
insmod all_video
insmod gfxterm
insmod png
insmod part_gpt
insmod part_msdos
insmod fat
insmod iso9660
insmod linux
insmod normal
insmod configfile
terminal_output gfxterm

menuentry "Materialized Linux 0.2 (live)" {
    linux /live/vmlinuz boot=live components hostname=materialized username=materialized locales=en_US.UTF-8 keyboard-layouts=us timezone=UTC console=tty0 console=ttyS0,115200n8
    initrd /live/initrd.img
}
menuentry "Materialized Linux 0.2 (safe graphics)" {
    linux /live/vmlinuz boot=live components hostname=materialized username=materialized nomodeset console=tty0 console=ttyS0,115200n8
    initrd /live/initrd.img
}
EOF
# GRUB's BIOS prefix is /boot/grub/i386-pc.
cp "$CHROOT/usr/lib/grub/i386-pc/"*.mod "$BINARY/boot/grub/i386-pc/"
cp "$CHROOT/usr/lib/grub/i386-pc/"*.lst "$BINARY/boot/grub/i386-pc/"
cp "$CHROOT/usr/share/grub/"*.pf2 "$BINARY/boot/grub/" 2>/dev/null || true
CORE="$(mktemp /var/tmp/materialized-grub-core.XXXXXX)"
trap 'rm -f "$CORE"' EXIT
BIOS_CONFIG="$(mktemp /var/tmp/materialized-grub-bios.XXXXXX.cfg)"
trap 'rm -f "$CORE" "$BIOS_CONFIG"' EXIT
cat > "$BIOS_CONFIG" <<'EOF'
set default=0
set timeout=0
search --no-floppy --file --set=root /live/filesystem.squashfs
linux /live/vmlinuz boot=live components hostname=materialized username=materialized locales=en_US.UTF-8 keyboard-layouts=us timezone=UTC console=tty0 console=ttyS0,115200n8
initrd /live/initrd.img
boot
EOF
grub-mkimage -d "$CHROOT/usr/lib/grub/i386-pc" -o "$CORE" -O i386-pc -p /boot/grub/i386-pc -c "$BIOS_CONFIG" biosdisk iso9660 normal configfile linux echo test search search_fs_file search_fs_uuid part_gpt part_msdos fat ext2 all_video gfxterm png jpeg
cat "$CHROOT/usr/lib/grub/i386-pc/cdboot.img" "$CORE" > "$BINARY/boot/grub/grub_eltorito"

# Embed a tiny configuration which finds the same configuration on UEFI.
EFI_CONFIG="$(mktemp /var/tmp/materialized-grub-efi.XXXXXX.cfg)"
trap 'rm -f "$CORE" "$EFI_CONFIG"' EXIT
cat > "$EFI_CONFIG" <<'EOF'
search --no-floppy --file --set=root /live/filesystem.squashfs
normal ($root)/boot/grub/grub.cfg
EOF
grub-mkstandalone --format=x86_64-efi --output="$BINARY/EFI/BOOT/BOOTX64.EFI" \
    --locales= --fonts= "boot/grub/grub.cfg=$EFI_CONFIG"
EFI_IMAGE="$(mktemp /var/tmp/materialized-efi-image.XXXXXX)"
trap 'rm -f "$CORE" "$EFI_CONFIG" "$EFI_IMAGE"' EXIT
truncate -s 4M "$EFI_IMAGE"
mkfs.vfat -n MAT_EFI "$EFI_IMAGE" >/dev/null
mmd -i "$EFI_IMAGE" ::/EFI ::/EFI/BOOT
mcopy -i "$EFI_IMAGE" "$BINARY/EFI/BOOT/BOOTX64.EFI" ::/EFI/BOOT/BOOTX64.EFI
cp "$EFI_IMAGE" "$BINARY/efiboot.img"

xorriso -as mkisofs \
    -iso-level 3 -full-iso9660-filenames -joliet -rational-rock \
    -volid MATERIALIZED_0_2 \
    -appid "Materialized Linux 0.2.1" \
    -publisher "Materialized Linux contributors" \
    -output "$OUT" \
    -c boot.catalog \
    -b boot/grub/grub_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot -e efiboot.img -no-emul-boot \
    -isohybrid-gpt-basdat -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin \
    "$BINARY" >/var/tmp/materialized-xorriso.log 2>&1
sha256sum "$OUT" > "$OUT.sha256"
printf 'Materialized ISO: %s\n' "$OUT"
printf 'SHA-256: %s\n' "$(cut -d' ' -f1 "$OUT.sha256")"
xorriso -indev "$OUT" -report_el_torito plain 2>/dev/null
