#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-bios}"
DEVICE="${2:-VGA}"
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
MONITOR="/var/tmp/materialized-$MODE-monitor.sock"
SCREEN="/var/tmp/materialized-$MODE-screen.ppm"
rm -f "$MONITOR" "$SCREEN" "build/vm/firmware-$MODE.log"
qemu-system-x86_64 -machine q35,accel=kvm -cpu host -m 2048 -smp 2 \
    -cdrom build/iso/materialized-0.2.0-amd64.iso -boot d -display none \
    -serial "file:build/vm/firmware-$MODE.log" \
    -monitor "unix:$MONITOR,server=on,wait=off" \
    -device "$DEVICE" -device virtio-rng-pci -no-reboot -no-shutdown \
    >"/var/tmp/materialized-$MODE-qemu.log" 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true' EXIT
sleep 65
printf 'screendump %s\n' "$SCREEN" | socat - "UNIX-CONNECT:$MONITOR" >/dev/null
sleep 1
convert "$SCREEN" "build/vm/firmware-$MODE.png"
printf 'Captured %s with %s\n' "build/vm/firmware-$MODE.png" "$DEVICE"
