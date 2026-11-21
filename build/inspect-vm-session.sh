#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
rm -f /var/tmp/materialized-debug-monitor.sock /var/tmp/materialized-debug-screen.ppm build/vm/debug-uefi.log
qemu-system-x86_64 -machine q35,accel=kvm -cpu host -m 2048 -smp 2 \
    -cdrom build/iso/materialized-0.2.0-amd64.iso -boot d -display none \
    -serial file:build/vm/debug-uefi.log \
    -monitor unix:/var/tmp/materialized-debug-monitor.sock,server=on,wait=off \
    -device virtio-vga -device virtio-rng-pci -no-reboot -no-shutdown \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    >/var/tmp/materialized-debug-qemu.log 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true' EXIT
sleep 95
printf 'sendkey ctrl-alt-f2\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 3
printf 'sendkey m\nsendkey a\nsendkey t\nsendkey e\nsendkey r\nsendkey i\nsendkey a\nsendkey l\nsendkey i\nsendkey z\nsendkey e\nsendkey d\nsendkey ret\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 1
printf 'sendkey m\nsendkey a\nsendkey t\nsendkey e\nsendkey r\nsendkey i\nsendkey a\nsendkey l\nsendkey i\nsendkey z\nsendkey e\nsendkey d\nsendkey ret\n' | socat - UNIX-CONNECT:/var/tmp/materialized-debug-monitor.sock >/dev/null
sleep 1
python3 - <<'PY'
import socket
import time
command = 'ps -ef | grep -E "matcomp|matshell|matsessiond"; cat /tmp/materialized-session.log; ls -l /dev/ttyS0\n'
with socket.socket(socket.AF_UNIX) as monitor:
    monitor.connect('/var/tmp/materialized-debug-monitor.sock')
    for char in command:
        key = {' ': 'spc', '/': 'slash', '.': 'dot', '-': 'minus', '|': 'shift-backslash', '>': 'shift-dot', ';': 'semicolon', '"': 'shift-apostrophe', '\n': 'ret'}.get(char, char)
        monitor.sendall(f'sendkey {key}\n'.encode())
        time.sleep(0.008)
time.sleep(3)
with socket.socket(socket.AF_UNIX) as monitor:
    monitor.connect('/var/tmp/materialized-debug-monitor.sock')
    monitor.sendall(b'screendump /var/tmp/materialized-debug-screen.ppm\n')
PY
convert /var/tmp/materialized-debug-screen.ppm build/vm/debug-processes.png
printf 'Captured build/vm/debug-processes.png\n'
