#!/usr/bin/env bash
# Boot the ISO, open Firefox via hotkey, then use the REAL terminal (Ctrl+Alt+T)
# to cat the browser log on screen; capture it as a screenshot.
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
RT=/var/tmp/materialized-browserdbg
mkdir -p "$RT"
MON="$RT/monitor.sock"; SER="$RT/serial.sock"; LOG="$ROOT/build/vm/browserdbg.log"
rm -f "$MON" "$SER" "$LOG"
qemu-system-x86_64 -machine q35,accel=kvm -cpu host -m 3072 -smp 2 \
    -cdrom "$ROOT/build/iso/materialized-0.2.0-amd64.iso" -boot d -display none \
    -chardev "socket,id=serial0,path=$SER,server=on,wait=off" -serial chardev:serial0 \
    -monitor "unix:$MON,server=on,wait=off" \
    -device virtio-vga -device virtio-rng-pci -no-reboot -no-shutdown \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    >"$RT/qemu.log" 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true' EXIT
for _ in $(seq 1 50); do [ -S "$SER" ] && break; sleep 0.1; done
socat -u "UNIX-CONNECT:$SER" "OPEN:$LOG,creat,trunc" >/dev/null 2>&1 &
SOCAT=$!
trap 'kill "$PID" "$SOCAT" 2>/dev/null || true; wait "$PID" 2>/dev/null || true' EXIT
for _ in $(seq 1 240); do
    grep -a -q 'Materialized session started' "$LOG" 2>/dev/null && break
    sleep 1
done
sleep 3
key() { printf 'sendkey %s\n' "$1" | socat - "UNIX-CONNECT:$MON" >/dev/null; }
type_cmd() {
    local s="$1"
    for ((i=0; i<${#s}; i++)); do
        local c="${s:$i:1}"
        case "$c" in
            ' ') key spc ;; '/') key slash ;; '.') key dot ;; '-') key minus ;;
            '_') key shift-minus ;; ':') key shift-semicolon ;; '=') key equal ;;
            *) key "$c" ;;
        esac
        sleep 0.03
    done
    key ret
}
key ctrl-alt-b
sleep 25
key ctrl-alt-t
sleep 4
type_cmd "cat /tmp/matbrowser.log"
sleep 2
type_cmd "ls /tmp"
sleep 2
printf 'screendump %s/screen.ppm\n' "$RT" | socat - "UNIX-CONNECT:$MON" >/dev/null
sleep 1
python3 "$ROOT/build/ppm2png.py" "$RT/screen.ppm" "$ROOT/build/vm/browserdbg.png"
printf 'captured build/vm/browserdbg.png\n'
printf '== serial tail ==\n'
tail -12 "$LOG"
