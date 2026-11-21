#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
rm -f build/vm/debug-uefi.log /var/tmp/materialized-debug-monitor.sock /var/tmp/materialized-debug-screen.ppm
qemu-system-x86_64 \
    -machine q35,accel=kvm -cpu host -m 2048 -smp 2 \
    -cdrom build/iso/materialized-0.2.0-amd64.iso -boot d \
    -display none -serial file:build/vm/debug-uefi.log \
    -monitor unix:/var/tmp/materialized-debug-monitor.sock,server=on,wait=off \
    -device virtio-vga -device virtio-rng-pci -no-reboot -no-shutdown \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    >/var/tmp/materialized-debug-qemu.log 2>&1 &
PID=$!
cleanup() { kill "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true; }
trap cleanup EXIT
sleep 95
printf 'screendump /var/tmp/materialized-debug-screen.ppm\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
convert /var/tmp/materialized-debug-screen.ppm build/vm/debug-uefi.png
printf 'sendkey ctrl-alt-f2\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 2
printf 'sendkey m\nsendkey a\nsendkey t\nsendkey e\nsendkey r\nsendkey i\nsendkey a\nsendkey l\nsendkey i\nsendkey z\nsendkey e\nsendkey d\nsendkey ret\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 1
printf 'sendkey m\nsendkey a\nsendkey t\nsendkey e\nsendkey r\nsendkey i\nsendkey a\nsendkey l\nsendkey i\nsendkey z\nsendkey e\nsendkey d\nsendkey ret\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 2
printf 'sendkey c\nsendkey a\nsendkey t\nsendkey spc\nsendkey slash\nsendkey t\nsendkey m\nsendkey p\nsendkey slash\nsendkey m\nsendkey a\nsendkey t\nsendkey e\nsendkey r\nsendkey i\nsendkey a\nsendkey l\nsendkey i\nsendkey z\nsendkey e\nsendkey d\nsendkey minus\nsendkey s\nsendkey e\nsendkey s\nsendkey s\nsendkey i\nsendkey o\nsendkey n\nsendkey dot\nsendkey l\nsendkey o\nsendkey g\nsendkey ret\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 2
printf 'screendump /var/tmp/materialized-debug-screen.ppm\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
convert /var/tmp/materialized-debug-screen.ppm build/vm/debug-tty2.png
printf 'Captured build/vm/debug-uefi.png and debug-tty2.png\n'
