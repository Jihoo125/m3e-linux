#!/usr/bin/env bash
# Build a real Debian 13 (trixie) x86_64 live ISO for Materialized Linux.
#
# Requires root, a Linux host, debootstrap, live-build, xorrisofs, squashfs-tools
# and GRUB's EFI and BIOS binaries. Network access is needed for Debian packages.
# Run from the repository root: sudo bash platform/iso/build.sh [--minimal]
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="${MAT_ISO_OUTPUT:-$ROOT/build/iso/materialized-0.2.1-amd64.iso}"
WORK="${MAT_ISO_WORK:-/var/tmp/materialized-iso}"
BUILD_ROOT="${MAT_BUILD_ROOT:-$ROOT/build}"
MIRROR="${MAT_DEBIAN_MIRROR:-http://deb.debian.org/debian}"
SNAPSHOT="${MAT_DEBIAN_SNAPSHOT:-}"
BOOTSTRAP_KEYRING="${DEBOOTSTRAP_KEYRING:-/usr/local/share/keyrings/debian-archive-keyring-current.gpg}"
if [ ! -r "$BOOTSTRAP_KEYRING" ]; then
    echo "Run build/refresh-debian-keyring.sh before building the ISO" >&2
    exit 1
fi
# live-build on older Ubuntu resets the keyring to this path during defaults.
# Install the already verified keyring there, preserving all signature checks.
install -Dm0644 "$BOOTSTRAP_KEYRING" /usr/share/keyrings/debian-archive-keyring.gpg
# Older cdebootstrap also reads a suite-specific keyring.
install -Dm0644 "$BOOTSTRAP_KEYRING" /usr/share/keyrings/debian-archive-trixie-keyring.gpg
install -Dm0644 "$BOOTSTRAP_KEYRING" /usr/share/keyrings/debian-keyring.gpg

VARIANT="${1:-standard}"

if [ "$(id -u)" -ne 0 ]; then
    echo "ISO build must run as root; invoke with sudo." >&2
    exit 1
fi
if [ "$(uname -m)" != "x86_64" ]; then
    echo "The live ISO is x86_64-only." >&2
    exit 1
fi
for tool in debootstrap lb xorrisofs mksquashfs grub-mkimage; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "Missing ISO tool: $tool (run build/bootstrap-wsl.sh --iso)" >&2
        exit 1
    }
done

# Ubuntu 22.04 ships an older debootstrap without a trixie alias. Debian's
# generic sid script is the supported bootstrap recipe for the named suite.
# Create the alias only when it is absent; never replace host scripts.
if [ ! -e /usr/share/debootstrap/scripts/trixie ]; then
    if [ -e /usr/share/debootstrap/scripts/sid ]; then
        ln -s sid /usr/share/debootstrap/scripts/trixie
        echo "Added debootstrap trixie alias (older host compatibility)"
    else
        echo "debootstrap lacks both trixie and generic sid scripts" >&2
        exit 1
    fi
fi
for file in matcomp/matcomp matsessiond/matsessiond matshell/matshell qml/bin/matgallery; do
    [ -x "$BUILD_ROOT/$file" ] || { echo "Build required: $file" >&2; exit 1; }
done
for app in matcalc mattext matsysmon matfiles matsettings matterminal matview matshot matcal; do
    [ -x "$BUILD_ROOT/$app/$app" ] || { echo "Build required: $app" >&2; exit 1; }
done

export MAT_STAGING="$WORK/session-root"
rm -rf "$WORK"
mkdir -p "$WORK" "$(dirname "$OUT")"
bash "$ROOT/platform/packages/stage-session.sh"

mkdir -p "$WORK/live"
LB_CONFIG=(
    --distribution trixie
    --mode debian
    --parent-distribution trixie
    --initramfs live-boot
    --initsystem systemd
    --bootloader grub2
    --architectures amd64
    --bootstrap-keyring /usr/share/keyrings/debian-archive-keyring.gpg
    --binary-images iso-hybrid
    --debian-installer false
    --memtest none
    --archive-areas "main contrib non-free-firmware"
    --keyring-packages debian-archive-keyring
    --mirror-bootstrap "$MIRROR"
    --mirror-binary "$MIRROR"
    --mirror-chroot "$MIRROR"
    --mirror-chroot-security "http://security.debian.org/debian-security"
    --mirror-chroot-backports "$MIRROR"
    --apt-indices true
    --apt-recommends true
    --apt-secure true
    --checksums sha256
    --compression xz
    --debug
    --source false
    --bootappend-live "boot=live components hostname=materialized username=materialized locales=en_US.UTF-8 keyboard-layouts=us timezone=UTC quiet splash"
    --iso-application "Materialized Linux"
    --iso-publisher "Materialized Linux contributors"
    --iso-volume "MATERIALIZED_0_1"
    --linux-packages "linux-image"
    --linux-flavours "amd64"
    --firmware-binary false
    --firmware-chroot false
    --security false
)
if [ -n "$SNAPSHOT" ]; then
    LB_CONFIG+=(--mirror-bootstrap "$SNAPSHOT" --mirror-chroot "$SNAPSHOT" --mirror-binary "$SNAPSHOT")
fi

(
    cd "$WORK"
    lb config "${LB_CONFIG[@]}"
)
# Older live-build writes an auto-selected initramfs/init system into config
# after reading command-line options. Set them explicitly before lb build.
python3 - "$WORK/config/common" <<'PY'
from pathlib import Path
import re
import sys
path = Path(sys.argv[1])
text = path.read_text()
for key, value in {'LB_INITRAMFS': 'live-boot', 'LB_INITSYSTEM': 'systemd'}.items():
    if re.search(rf'^{key}=', text, re.MULTILINE):
        text = re.sub(rf'^{key}=.*$', f'{key}="{value}"', text, flags=re.MULTILINE)
    else:
        text += f'\n{key}="{value}"\n'
path.write_text(text)
PY

mkdir -p "$WORK/config/package-lists"
cat > "$WORK/config/package-lists/materialized.list.chroot" <<'EOF'
# Materialized desktop runtime and live-session utilities
systemd
systemd-sysv
dbus
sudo
locales
ca-certificates
initramfs-tools
linux-image-amd64
firmware-linux-free
firmware-linux-nonfree
firmware-amd-graphics
firmware-intel-graphics
firmware-iwlwifi
firmware-realtek
firmware-misc-nonfree
mesa-utils
mesa-vulkan-drivers
mesa-va-drivers
mesa-vdpau-drivers
qml6-module-qtquick
qml6-module-qtquick-controls
qml6-module-qtquick-layouts
qml6-module-qtquick-window
qml6-module-qtquick-templates
qml6-module-qtquick-shapes
qml6-module-qtqml-workerscript
qml6-module-qtquick-dialogs
qml6-module-qtquick-localstorage
qml6-module-qt-labs-platform
live-boot
live-config
live-config-systemd
live-config-doc
live-tools
user-setup
grub-efi-amd64-bin
grub-pc-bin
grub-common
efibootmgr
os-prober
kmod
udev
pciutils
usbutils
procps
psmisc
iproute2
iputils-ping
ethtool
network-manager
bluez
upower
udisks2
packagekit
pipewire
pipewire-pulse
wireplumber
xdg-utils
xdg-desktop-portal
firefox-esr
qt6-base-dev
qt6-declarative-dev
qt6-wayland-dev
libqt6svg6
libqt6waylandclient6
libqt6waylandcompositor6
# wlroots 0.15 runtime ABI for the compositor built by this project.
# It is copied by stage-session.sh because the Debian runtime is newer.
wayland-protocols
libinput10
libxkbcommon0
libxcb-res0
libdrm2
libegl1
libgl1-mesa-dri
libgles2
libpixman-1-0
fonts-inter-variable
fonts-roboto-unhinted
fonts-noto-core
fonts-noto-cjk
fonts-noto-color-emoji
adwaita-icon-theme
hicolor-icon-theme
dbus-x11
xwayland
xvfb
weston
vim-tiny
nano
less
file
btrfs-progs
e2fsprogs
dosfstools
parted
fdisk
gdisk
squashfs-tools
util-linux
mount
cryptsetup
keyutils
EOF

# Copy the locally built session into the live rootfs. This is not a mock desktop.
mkdir -p "$WORK/config/includes.chroot/etc/apt"
cat > "$WORK/config/includes.chroot/etc/apt/sources.list" <<'EOF'
deb http://deb.debian.org/debian trixie main contrib non-free-firmware
deb http://deb.debian.org/debian trixie-updates main contrib non-free-firmware
deb http://security.debian.org/debian-security trixie-security main contrib non-free-firmware
EOF
mkdir -p "$WORK/config/includes.chroot"
cp -a "$WORK/session-root/." "$WORK/config/includes.chroot/"

mkdir -p "$WORK/config/includes.chroot/usr/share/materialized/qml/Materialized"
# The staging root owns the generated QML import directory; ensure the layout
# expected by /usr/share/materialized/qml/Materialized is preserved.
if [ -d "$WORK/session-root/usr/share/materialized/qml/Materialized" ]; then
    cp -a "$WORK/session-root/usr/share/materialized/qml/Materialized/." \
          "$WORK/config/includes.chroot/usr/share/materialized/qml/Materialized/"
fi

mkdir -p "$WORK/config/hooks"
cat > "$WORK/config/hooks/010-materialized-session.chroot" <<'EOF'
#!/bin/sh
set -eu
export DEBIAN_FRONTEND=noninteractive
useradd --create-home --uid 1000 --shell /bin/bash materialized || true
usermod --append --groups sudo,audio,video,render,plugdev,dialout materialized || true
# A live serial getty can retain ttyS0. Provide an opt-in serial log device
# without relying on a login prompt or world-writable console.
cat > /etc/udev/rules.d/60-materialized-serial.rules <<'RULE'
KERNEL=="ttyS0", GROUP="dialout", MODE="0660"
RULE
printf 'materialized:materialized\n' | chpasswd
passwd -l root || true
mkdir -p /home/materialized/.config /home/materialized/.local/share /home/materialized/.cache
cp -a /etc/skel/. /home/materialized/ 2>/dev/null || true
chown -R materialized:materialized /home/materialized
mkdir -p /etc/systemd/system/getty@tty1.service.d
cat > /etc/systemd/system/getty@tty1.service.d/materialized-autologin.conf <<'UNIT'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin materialized --noclear %I $TERM
UNIT
cat > /home/materialized/.profile <<'PROFILE'
if [ "$(tty)" = "/dev/tty1" ] && [ -z "${WAYLAND_DISPLAY:-}" ] && [ -z "${DISPLAY:-}" ]; then
    exec /usr/bin/materialized-session >>/tmp/materialized-session.log 2>&1
fi
PROFILE
chown materialized:materialized /home/materialized/.profile
mkdir -p /etc/X11/xinit
cat > /home/materialized/.xinitrc <<'XINIT'
#!/bin/sh
export QT_QPA_PLATFORM=wayland
export QML2_IMPORT_PATH=/usr/share/materialized/qml
exec /usr/bin/materialized-session
XINIT
chmod 755 /home/materialized/.xinitrc
systemctl enable NetworkManager.service 2>/dev/null || true
systemctl enable bluetooth.service 2>/dev/null || true
systemctl --global enable pipewire.service pipewire-pulse.service wireplumber.service 2>/dev/null || true
systemctl enable systemd-timesyncd.service 2>/dev/null || true
# The live environment has no display manager. TTY1 is the recovery path.
systemctl set-default graphical.target
rm -rf /var/lib/apt/lists/*
EOF
chmod 755 "$WORK/config/hooks/010-materialized-session.chroot"

mkdir -p "$WORK/config/bootloaders/grub-pc"
cat > "$WORK/config/bootloaders/grub-pc/grub.cfg" <<'EOF'
set default=0
set timeout=4
set gfxpayload=keep
insmod all_video
insmod gfxterm
insmod png
insmod jpeg
insmod part_gpt
insmod part_msdos
insmod fat
insmod ext2
insmod iso9660
insmod loopback
insmod linux
insmod normal
insmod search
insmod search_fs_uuid
insmod search_fs_file
insmod search_label
insmod configfile
terminal_output gfxterm
set theme=/boot/grub/theme/materialized.txt

menuentry "Materialized Linux (live)" {
    linux /live/vmlinuz boot=live components quiet splash
    initrd /live/initrd.img
}
menuentry "Materialized Linux (safe graphics)" {
    linux /live/vmlinuz boot=live components nomodeset
    initrd /live/initrd.img
}
menuentry "Materialized Linux (recovery shell)" {
    linux /live/vmlinuz boot=live components single
    initrd /live/initrd.img
}
EOF
mkdir -p "$WORK/config/bootloaders/grub-efi"
cp "$WORK/config/bootloaders/grub-pc/grub.cfg" "$WORK/config/bootloaders/grub-efi/grub.cfg"
mkdir -p "$WORK/config/bootloaders/grub-pc/theme"
cat > "$WORK/config/bootloaders/grub-pc/theme/materialized.txt" <<'EOF'
title-text: "Materialized Linux"
title-font: "Unifont Regular 16"
title-color: "#E6E0E9"
message-font: "Unifont Regular 14"
message-color: "#E6E0E9"
message-bg-color: "#18120C"
desktop-image: "materialized-dawn.png"
desktop-color: "#18120C"
terminal-font: "Unifont Regular 14"
terminal-color: "#E6E0E9"
EOF
cp "$WORK/config/bootloaders/grub-pc/theme/materialized.txt" "$WORK/config/bootloaders/grub-efi/materialized.txt"
cp "$ROOT/design/wallpaper/materialized-dawn.png" "$WORK/config/bootloaders/grub-pc/theme/materialized-dawn.png"
cp "$ROOT/design/wallpaper/materialized-dawn.png" "$WORK/config/bootloaders/grub-efi/materialized-dawn.png"

# live-build on older hosts looks for a legacy Contents-${arch}.gz path. Modern
# Debian serves main/Contents-${arch}.gz. Patch only this project's helper copy.
mkdir -p "$WORK/local/live-build/scripts"
cp /usr/lib/live/build/lb_chroot_linux-image "$WORK/local/live-build/scripts/lb_chroot_linux-image"
python3 - "$WORK/local/live-build/scripts/lb_chroot_linux-image" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
s = s.replace('/dists/${LB_PARENT_DISTRIBUTION}/Contents-${LB_ARCHITECTURES}.gz',
              '/dists/${LB_PARENT_DISTRIBUTION}/main/Contents-${LB_ARCHITECTURES}.gz')
s = s.replace('/dists/${LB_DISTRIBUTION}/Contents-${LB_ARCHITECTURES}.gz',
              '/dists/${LB_DISTRIBUTION}/main/Contents-${LB_ARCHITECTURES}.gz')
p.write_text(s)
PY
chmod 755 "$WORK/local/live-build/scripts/lb_chroot_linux-image"

# Create the Debian base and install the session into its chroot. The old
# binary ISO stages are intentionally skipped: finalize-iso.sh supplies GRUB 2
# UEFI/BIOS support that the legacy host helper lacks.
(
    cd "$WORK"
    lb bootstrap
    lb chroot
)
# Finalize with GRUB 2 because the older host's own ISO stage is BIOS-only.
MAT_ISO_WORK="$WORK" MAT_ISO_OUTPUT="$OUT" \
    bash "$ROOT/platform/iso/finalize-iso.sh"
printf 'Materialized ISO: %s\n' "$OUT"
printf 'SHA-256: %s\n' "$(cut -d' ' -f1 "$OUT.sha256")"
