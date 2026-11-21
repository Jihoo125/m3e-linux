#!/usr/bin/env bash
# build/probe-host.sh — report what this machine can build and what is missing.
# Run inside WSL/Debian/Ubuntu:  bash build/probe-host.sh
set -u

ok()   { printf '  \033[32mok\033[0m      %s\n' "$1"; }
miss() { printf '  \033[31mMISSING\033[0m %s\n' "$1"; }
warn() { printf '  \033[33mwarn\033[0m    %s\n' "$1"; }

echo "== host =="
grep PRETTY_NAME /etc/os-release 2>/dev/null | cut -d= -f2 | tr -d '"' || echo "unknown"
printf '  kernel  %s\n' "$(uname -r)"
printf '  arch    %s\n' "$(uname -m)"
printf '  mem     %s MB\n' "$(( $(awk '/MemTotal/{print $2}' /proc/meminfo 2>/dev/null || echo 0) / 1024 ))"
printf '  cpus    %s\n' "$(nproc 2>/dev/null || echo '?')"

echo
echo "== toolchains =="
for t in bash make cmake meson ninja g++ gcc clang pkg-config python3 git \
         dpkg-deb debootstrap mksquashfs xorriso qemu-system-x86_64 wslpath \
         wayland-scanner glslangValidator; do
  if command -v "$t" >/dev/null 2>&1; then ok "$t -> $(command -v $t)"; else miss "$t"; fi
done

echo
echo "== versions =="
g++ --version 2>/dev/null | head -1 || true
cmake --version 2>/dev/null | head -1 || true
meson --version 2>/dev/null | head -1 || true
python3 --version 2>/dev/null || true

echo
echo "== Qt 6 dev packages =="
for p in qt6-base-dev qt6-declarative-dev qt6-wayland-dev qt6-tools-dev \
         qml6-module-qtquick-controls qml6-module-qtquick-templates \
         libqt6opengl6-dev qt6-svg-dev; do
  if dpkg -s "$p" >/dev/null 2>&1; then ok "$p"; else miss "$p"; fi
done

echo
echo "== wayland / compositor libs =="
for p in libwayland-dev wayland-protocols libwlroots-dev libxkbcommon-dev \
         libinput-dev libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev \
         libudev-dev libpixman-1-dev libseat-dev; do
  if dpkg -s "$p" >/dev/null 2>&1; then ok "$p"; else miss "$p"; fi
done

echo
echo "== image build tools =="
for p in debootstrap mksquashfs unsquashfs xorriso genisoimage mkisofs \
         sfdisk parted gdisk dosfstools btrfs-progs e2fsprogs cryptsetup \
         grub-mkimage grub-mkrescue shim-signed; do
  if command -v "$p" >/dev/null 2>&1; then ok "$p"; else miss "$p"; fi
done

echo
echo "== virtualisation (needed for tests/vm) =="
if [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
  ok "/dev/kvm accessible"
else
  warn "/dev/kvm not usable — QEMU will fall back to TCG (slow but functional)"
fi

echo
echo "== next step =="
echo "  Install what is missing with:  bash build/bootstrap-debian.sh"
