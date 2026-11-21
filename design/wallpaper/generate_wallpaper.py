#!/usr/bin/env python3
"""Create Materialized Dawn without requiring raster-image Python packages.

Uses a single zlib-compressed RGB scanline buffer and standard-library CRC.
This is intentionally a small generator, not a web/image-mockup asset.
"""
from __future__ import annotations
import binascii
import math
import struct
import zlib
from pathlib import Path

WIDTH, HEIGHT = 1920, 1080


def chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', binascii.crc32(kind + payload) & 0xFFFFFFFF)


def main() -> None:
    out = Path(__file__).resolve().parent
    out.mkdir(parents=True, exist_ok=True)
    data = bytearray()
    for y in range(HEIGHT):
        data.append(0)
        for x in range(WIDTH):
            t = x / WIDTH * 0.6 + y / HEIGHT * 0.4
            glow = max(0.0, 1 - math.hypot((x / WIDTH - .74) / .75, (y / HEIGHT - .20) / .9))
            data.extend((
                min(255, int(21 + 169 * t + 60 * glow)),
                min(255, int(43 + 103 * t + 35 * glow)),
                min(255, int(53 + 54 * t + 22 * glow)),
            ))
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', WIDTH, HEIGHT, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(bytes(data), 9))
    png += chunk(b'IEND', b'')
    path = out / 'materialized-dawn.png'
    path.write_bytes(png)
    print(f'{path}: {WIDTH}x{HEIGHT}, {len(png)} bytes')


if __name__ == '__main__':
    main()
