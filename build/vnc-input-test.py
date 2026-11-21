#!/usr/bin/env python3
"""Minimal RFB 3.8 client for VMware Workstation's remote display.

Proves real keyboard/pointer input on VMware without VMware Tools: launches the
terminal with Ctrl+Alt+T, types a command that writes to the guest serial
console, and moves the pointer. The caller gates on the serial log.

VNC authentication (type 2) is DES-ECB over the 16-byte challenge with
bit-reversed key bytes. DES is implemented here (self-tested against published
vectors) so the client has no external crypto dependency.
"""
import socket
import struct
import sys
import time

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5901
PASSWORD = sys.argv[3] if len(sys.argv) > 3 else "matvm123"

_PC1 = [57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34, 26, 18,
        10, 2, 59, 51, 43, 35, 27, 19, 11, 3, 60, 52, 44, 36,
        63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22,
        14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 28, 20, 12, 4]
_PC2 = [14, 17, 11, 24, 1, 5, 3, 28, 15, 6, 21, 10, 23, 19, 12, 4,
        26, 8, 16, 7, 27, 20, 13, 2, 41, 52, 31, 37, 47, 55, 30, 40,
        51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32]
_IP = [58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
       62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
       57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
       61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7]
_FP = [40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
       38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
       36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
       34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25]
_E = [32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11, 12, 13,
      12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
      24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1]
_P = [16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10,
      2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25]
_SHIFTS = [1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1]
_SBOX = [
    [[14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7],
     [0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8],
     [4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0],
     [15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13]],
    [[15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10],
     [3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5],
     [0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15],
     [13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9]],
    [[10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8],
     [13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1],
     [13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7],
     [1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12]],
    [[7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15],
     [13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9],
     [10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4],
     [3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14]],
    [[2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9],
     [14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6],
     [4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14],
     [11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3]],
    [[12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11],
     [10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8],
     [9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6],
     [4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13]],
    [[4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1],
     [13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6],
     [1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2],
     [6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12]],
    [[13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7],
     [1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2],
     [7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8],
     [2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11]],
]


def _permute(value, table, in_bits):
    out = 0
    for pos in table:
        out = (out << 1) | ((value >> (in_bits - pos)) & 1)
    return out


def _rotl28(value, n):
    return ((value << n) | (value >> (28 - n))) & 0xFFFFFFF


def _subkeys(key8):
    key = int.from_bytes(key8, "big")
    cd = _permute(key, _PC1, 64)
    c, d = cd >> 28, cd & 0xFFFFFFF
    keys = []
    for shift in _SHIFTS:
        c, d = _rotl28(c, shift), _rotl28(d, shift)
        keys.append(_permute((c << 28) | d, _PC2, 56))
    return keys


def des_block(block8, key8):
    """Single DES block, ECB, no padding (exactly what VNC auth needs)."""
    keys = _subkeys(key8)
    state = _permute(int.from_bytes(block8, "big"), _IP, 64)
    left, right = state >> 32, state & 0xFFFFFFFF
    for k in keys:
        expanded = _permute(right, _E, 32) ^ k
        s_out = 0
        for i in range(8):
            chunk = (expanded >> (42 - 6 * i)) & 0x3F
            row = ((chunk & 0x20) >> 4) | (chunk & 1)
            col = (chunk >> 1) & 0xF
            s_out = (s_out << 4) | _SBOX[i][row][col]
        left, right = right, left ^ _permute(s_out, _P, 32)
    return _permute((right << 32) | left, _FP, 64).to_bytes(8, "big")


# Published DES known-answer tests: refuse to run if the implementation is wrong.
assert des_block(bytes(8), bytes(8)) == bytes.fromhex("8ca64de9c1b123a7")
assert des_block(bytes.fromhex("ffffffffffffffff"),
                 bytes.fromhex("ffffffffffffffff")) == bytes.fromhex("7359b2163e4edc58")


def vnc_des_response(challenge, password):
    key = password.encode()[:8].ljust(8, b"\0")
    reversed_key = bytes(int(f"{b:08b}"[::-1], 2) for b in key)
    return b"".join(des_block(challenge[i:i + 8], reversed_key)
                    for i in range(0, 16, 8))


def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise RuntimeError("connection closed during handshake")
        buf += chunk
    return buf


def key_event(sock, keysym, down):
    sock.sendall(struct.pack(">BBHI", 4, 1 if down else 0, 0, keysym))


def tap(sock, keysym, hold=0.05):
    key_event(sock, keysym, True)
    time.sleep(hold)
    key_event(sock, keysym, False)
    time.sleep(0.03)


def pointer_event(sock, x, y, mask=0):
    sock.sendall(struct.pack(">BBHH", 5, mask, x, y))


def main():
    sock = socket.create_connection((HOST, PORT), timeout=30)
    sock.settimeout(30)
    version = recv_exact(sock, 12)
    print("server version:", version.decode().strip())
    sock.sendall(b"RFB 003.008\n")
    ntypes = recv_exact(sock, 1)[0]
    types = recv_exact(sock, ntypes)
    if 2 not in types:
        raise RuntimeError(f"server offers no VNC auth: {types!r}")
    sock.sendall(bytes([2]))
    challenge = recv_exact(sock, 16)
    sock.sendall(vnc_des_response(challenge, PASSWORD))
    result = struct.unpack(">I", recv_exact(sock, 4))[0]
    if result != 0:
        raise RuntimeError(f"VNC authentication failed: {result}")
    sock.sendall(bytes([1]))  # ClientInit: shared
    init = recv_exact(sock, 24)
    width, height = struct.unpack(">HH", init[:4])
    namelen = struct.unpack(">I", init[20:24])[0]
    name = recv_exact(sock, namelen).decode(errors="replace")
    print(f"framebuffer {width}x{height} name={name!r}")

    # Ctrl+Alt+T: compositor hotkey launches the real PTY terminal.
    key_event(sock, 0xFFE3, True)   # Control_L
    key_event(sock, 0xFFE9, True)   # Alt_L
    tap(sock, ord("t"))
    key_event(sock, 0xFFE9, False)
    key_event(sock, 0xFFE3, False)
    time.sleep(6)

    # Type a command whose output lands on the guest serial console.
    for ch in "echo input-ok > /dev/ttyS0":
        tap(sock, ord(ch))
    tap(sock, 0xFF0D)  # Return
    time.sleep(2)

    # Move the pointer so the software cursor is inside the frame.
    pointer_event(sock, width // 2 + 60, height // 2 + 40)
    time.sleep(0.2)
    pointer_event(sock, width // 2 + 80, height // 2 + 50)
    time.sleep(0.5)
    sock.close()
    print("VMware VNC input sequence sent (hotkey, typing, pointer).")


if __name__ == "__main__":
    main()