#!/usr/bin/env bash
# Boot an ISO in QEMU and verify the live session's serial marker.
# Usage: tests/vm/run-qemu.sh --iso path.iso [--mode uefi|bios] [--keep-running]
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ISO=""
MODE="uefi"
KEEP=0
TIMEOUT="${MAT_QEMU_TIMEOUT:-300}"
OUT_DIR="$ROOT/build/vm"
RUNTIME_DIR="${MAT_QEMU_RUNTIME:-/var/tmp/materialized-qemu-$UID}"
mkdir -p "$OUT_DIR" "$RUNTIME_DIR"
chmod 700 "$RUNTIME_DIR"
MONITOR="$RUNTIME_DIR/monitor-$MODE.sock"
SERIALSOCK="$RUNTIME_DIR/serial-$MODE.sock"
while [ "$#" -gt 0 ]; do
    case "$1" in
        --iso) ISO="$2"; shift 2 ;;
        --mode) MODE="$2"; shift 2 ;;
        --keep-running) KEEP=1; shift ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 --iso FILE [--mode uefi|bios] [--keep-running] [--timeout SECONDS]"
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done
[ -n "$ISO" ] && [ -s "$ISO" ] || { echo "ISO does not exist: $ISO" >&2; exit 2; }
command -v qemu-system-x86_64 >/dev/null 2>&1 || { echo "Install qemu-system-x86" >&2; exit 2; }
[ "$MODE" = uefi ] || [ "$MODE" = bios ] || { echo "--mode must be uefi or bios" >&2; exit 2; }
mkdir -p "$OUT_DIR"
LOG="$OUT_DIR/boot-$MODE.log"
SCREEN="$OUT_DIR/boot-$MODE.ppm"
PIDFILE="$OUT_DIR/qemu-$MODE.pid"
rm -f "$LOG" "$SCREEN" "$PIDFILE" "$MONITOR" "$SERIALSOCK"

ACCEL=tcg
CPU=max
if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then ACCEL=kvm; CPU=host;
else echo "Note: /dev/kvm not usable by this user; using slow TCG emulation (run with sudo for KVM)." >&2; fi
QEMU=(
    qemu-system-x86_64
    -machine q35,accel="$ACCEL"
    -cpu "$CPU"
    -m 3072
    -smp 2
    -cdrom "$ISO"
    -boot d
    -display none
    -chardev "socket,id=serial0,path=$SERIALSOCK,server=on,wait=off"
    -serial chardev:serial0
    -monitor "unix:$MONITOR,server=on,wait=off"
    -device virtio-vga
    -device virtio-rng-pci
    -no-reboot
    -no-shutdown
)
if [ "$MODE" = uefi ]; then
    if [ -f /usr/share/OVMF/OVMF_CODE_4M.fd ]; then
        FIRMWARE=/usr/share/OVMF/OVMF_CODE_4M.fd
    elif [ -f /usr/share/OVMF/OVMF_CODE.fd ]; then
        FIRMWARE=/usr/share/OVMF/OVMF_CODE.fd
    else
        echo "OVMF firmware not found; install ovmf" >&2
        exit 2
    fi
    cp "$FIRMWARE" "$OUT_DIR/OVMF_CODE-$MODE.fd"
    if [ -f /usr/share/OVMF/OVMF_VARS_4M.fd ]; then
        cp /usr/share/OVMF/OVMF_VARS_4M.fd "$OUT_DIR/OVMF_VARS-$MODE.fd"
    elif [ -f /usr/share/OVMF/OVMF_VARS.fd ]; then
        cp /usr/share/OVMF/OVMF_VARS.fd "$OUT_DIR/OVMF_VARS-$MODE.fd"
    fi
    QEMU+=(-drive "if=pflash,format=raw,readonly=on,file=$OUT_DIR/OVMF_CODE-$MODE.fd")
    if [ -f "$OUT_DIR/OVMF_VARS-$MODE.fd" ]; then
        QEMU+=(-drive "if=pflash,format=raw,file=$OUT_DIR/OVMF_VARS-$MODE.fd")
    fi
fi

"${QEMU[@]}" >/dev/null 2>&1 &
QEMU_PID=$!
echo "$QEMU_PID" > "$PIDFILE"
# Capture the serial stream live (the file chardev buffers and loses the tail
# when QEMU is terminated, hiding application-phase diagnostics).
for _ in $(seq 1 50); do [ -S "$SERIALSOCK" ] && break; sleep 0.1; done
socat -u "UNIX-CONNECT:$SERIALSOCK" "OPEN:$LOG,creat,trunc" >/dev/null 2>&1 &
SOCAT_PID=$!
cleanup() {
    if [ "$KEEP" -eq 0 ]; then
        kill "$QEMU_PID" 2>/dev/null || true
        wait "$QEMU_PID" 2>/dev/null || true
        kill "$SOCAT_PID" 2>/dev/null || true
        wait "$SOCAT_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

FOUND=0
for _ in $(seq 1 "$TIMEOUT"); do
    if ! kill -0 "$QEMU_PID" 2>/dev/null; then
        echo "QEMU exited before the session marker appeared; see $LOG" >&2
        tail -80 "$LOG" 2>/dev/null || true
        exit 1
    fi
    if [ -f "$LOG" ] && [ -s "$LOG" ] && grep -a -q 'Materialized session started' "$LOG" && grep -a -q 'Shell visible=1 exposed=1' "$LOG"; then
        FOUND=1
        break
    fi
    sleep 1
done

if [ "$FOUND" -ne 1 ]; then
    echo "Timed out waiting for Materialized session marker; see $LOG" >&2
    tail -80 "$LOG" 2>/dev/null || true
    exit 1
fi

printf 'Materialized live session marker found in %s\n' "$LOG"
# Give the first QML frame a moment to reach the virtual display before capture.
sleep 3
if [ -S "$MONITOR" ] && command -v socat >/dev/null 2>&1; then
    printf 'screendump %s\n' "$SCREEN" | socat - "UNIX-CONNECT:$MONITOR" >/dev/null || true
    # The monitor returns before the image has necessarily been flushed.
    for _ in $(seq 1 20); do
        [ -s "$SCREEN" ] && sleep 0.2
        if [ -s "$SCREEN" ]; then break; fi
        sleep 0.2
    done
fi
if [ -s "$SCREEN" ]; then
    python3 "$ROOT/build/ppm2png.py" "$SCREEN" "${SCREEN%.ppm}.png"
    printf 'Desktop screenshot captured: %s\n' "${SCREEN%.ppm}.png"
else
    echo 'No desktop screenshot was captured.' >&2
    exit 1
fi
# Launch real applications through compositor hotkeys and capture them:
# Ctrl+Alt+T opens the PTY terminal, Ctrl+Alt+B opens Firefox ESR.
if [ -S "$MONITOR" ] && command -v socat >/dev/null 2>&1; then
    printf 'sendkey ctrl-alt-t\n' | socat - "UNIX-CONNECT:$MONITOR" >/dev/null || true
    sleep 5
    printf 'sendkey ctrl-alt-b\n' | socat - "UNIX-CONNECT:$MONITOR" >/dev/null || true
    sleep 60
    APPS="$OUT_DIR/apps-$MODE.ppm"
    printf 'screendump %s\n' "$APPS" | socat - "UNIX-CONNECT:$MONITOR" >/dev/null || true
    sleep 1
    if [ -s "$APPS" ]; then
        python3 "$ROOT/build/ppm2png.py" "$APPS" "${APPS%.ppm}.png"
        printf 'Application screenshot captured: %s\n' "${APPS%.ppm}.png"
    fi
fi
if [ "$KEEP" -eq 1 ]; then
    printf 'QEMU is still running (pid %s). Monitor: %s\n' "$QEMU_PID" "$MONITOR"
    wait "$QEMU_PID"
else
    printf 'Boot-to-session smoke test passed (%s).\n' "$MODE"
fi
