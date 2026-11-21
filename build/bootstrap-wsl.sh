#!/usr/bin/env bash
# build/bootstrap-wsl.sh — install everything needed to develop Materialized Linux
# inside WSL2 (Ubuntu 22.04/24.04) or a Debian 13 container.
#
# Usage:  bash build/bootstrap-wsl.sh [--core] [--iso] [--vm]
#   --core  toolchain + Qt6 + Wayland libs      (default; enough to build & unit test)
#   --iso   debootstrap/squashfs/xorriso/grub   (needed to build an actual ISO)
#   --vm    qemu + OVMF                          (needed to boot the ISO)
#
# Everything is installed as root via apt. Re-running is safe and idempotent.
set -euo pipefail

MODE_CORE=0; MODE_ISO=0; MODE_VM=0
if [ "$#" -eq 0 ]; then MODE_CORE=1; fi
while [ "$#" -gt 0 ]; do
  case "$1" in
    --core) MODE_CORE=1 ;;
    --iso)  MODE_ISO=1;  MODE_CORE=1 ;;
    --vm)   MODE_VM=1;   MODE_CORE=1 ;;
    --all)  MODE_CORE=1; MODE_ISO=1; MODE_VM=1 ;;
    *) echo "unknown flag: $1" >&2; exit 2 ;;
  esac
  shift
done

if [ "$(id -u)" -ne 0 ]; then
  exec sudo -E bash "$0" "${ORIG_ARGS[@]:---core}"
fi

echo "==> refreshing package lists"
export DEBIAN_FRONTEND=noninteractive
apt-get update -y

CORE_PKGS=(
  # toolchain
  build-essential gcc g++ clang cmake meson ninja-build pkg-config python3 python3-pip
  git curl wget ca-certificates gnupg rsync xz-utils zstd file bc
  # Qt 6 (shell + apps). Ubuntu 22.04 ships Qt 6.2; the PPA below lifts it to 6.7+
  qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools
  qt6-svg-dev libqt6opengl6-dev qt6-wayland-dev qt6-l10n-tools
  libqt6svg6-dev qt6-base-dev-tools libgl1-mesa-dev
  qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts
  qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtquick-shapes
  qml6-module-qtqml-workerscript qml6-module-qttest qml6-module-qtquick-effects
  qml6-module-qtquick-particles qml6-module-qt-labs-platform
  qml6-module-qtquick-dialogs qml6-module-qtquick-localstorage
  # Wayland / compositor
  libwayland-dev wayland-protocols libxkbcommon-dev libxkbcommon-x11-dev
  libinput-dev libudev-dev libdrm-dev libgbm-dev libegl1-mesa-dev
  libgles2-mesa-dev libpixman-1-dev libseat-dev libwlroots-dev
  # image / colour
  libpng-dev libjpeg-dev libwebp-dev libexiv2-dev liblcms2-dev
  # audio / desktop plumbing used by apps
  libpipewire-0.3-dev libpulse-dev libcanberra-dev
  # D-Bus / system libs used by settings + session daemon
  libdbus-1-dev libnm-dev libupower-glib-dev libgudev-1.0-dev
  libpolkit-gobject-1-dev libudisks2-dev libsecret-1-dev libpackagekit-glib2-dev
  # terminal
  libvte-2.91-dev
  # misc libs used across the tree
  libglib2.0-dev libgudev-1.0-dev libexif-dev libraw-dev libheif-dev
  libtag1-dev libical-dev libappstream-dev libflatpak-dev
  # testing
  catch2 xvfb weston sway xwayland mesa-utils
)

ISO_PKGS=(
  debootstrap squashfs-tools xorriso isolinux syslinux-common
  dosfstools e2fsprogs btrfs-progs cryptsetup parted gdisk fdisk util-linux
  grub-common grub-efi-amd64-bin grub-pc-bin mtools
  apt-utils reprepro dpkg-dev devscripts debhelper fakeroot
)

VM_PKGS=(
  qemu-system-x86 qemu-utils ovmf swtpm swtpm-tools
)

install_group() {
  local -n list=$1
  local label=$2
  echo "==> installing $label packages"
  # Install one-by-one so a single missing name cannot abort the whole group.
  local missing=()
  for p in "${list[@]}"; do
    if apt-get install -y --no-install-recommends "$p" >/dev/null 2>&1; then
      printf '   ok      %s\n' "$p"
    else
      printf '   FAILED  %s\n' "$p"
      missing+=("$p")
    fi
  done
  if [ "${#missing[@]}" -gt 0 ]; then
    echo "   note: ${#missing[@]} package(s) unavailable on this distro: ${missing[*]}"
    echo "   (Debian 13 has all of them; Ubuntu 22.04 lacks a few Qt 6.7 modules.)"
  fi
}

if [ "$MODE_CORE" = 1 ]; then install_group CORE_PKGS "core build"; fi
if [ "$MODE_ISO"  = 1 ]; then install_group ISO_PKGS  "ISO build";  fi
if [ "$MODE_VM"   = 1 ]; then install_group VM_PKGS   "VM test";    fi

echo
echo "==> pip extras (used by design-token tooling)"
python3 -m pip install --quiet --upgrade \
  "material-color-utilities>=0.11" pillow jsonschema pyyaml || \
  echo "   (pip extras optional — the C++ engine does not need them)"

echo
echo "==> KVM check"
if [ -w /dev/kvm ]; then
  echo "   /dev/kvm is writable — QEMU will use hardware acceleration."
else
  echo "   /dev/kvm not writable."
  echo "   On WSL2 enable nested virtualisation in %USERPROFILE%\\.wslconfig:"
  echo '       [wsl2]'
  echo '       nestedVirtualization=true'
  echo '   then run:  wsl --shutdown   and re-open the shell.'
  echo "   Without it QEMU falls back to TCG (~20x slower, still usable)."
fi

echo
echo "==> done. Verify with:  bash build/probe-host.sh"
echo "    Then build with:    cmake --preset dev && cmake --build --preset dev"
