#!/usr/bin/env python3
"""Convert a QEMU screendump (binary PPM/P6) to PNG without external deps."""
import struct
import sys
import zlib
from pathlib import Path

raw = Path(sys.argv[1]).read_bytes()
parts = raw.split(maxsplit=4)
if len(parts) != 5 or parts[0] != b'P6':
    raise SystemExit('not a complete PPM image')
width, height = int(parts[1]), int(parts[2])
pixels = parts[4]
if len(pixels) != width * height * 3:
    raise SystemExit('incomplete PPM pixel data')


def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)


scanlines = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
png = (b'\x89PNG\r\n\x1a\n'
       + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
       + chunk(b'IDAT', zlib.compress(scanlines))
       + chunk(b'IEND', b''))
Path(sys.argv[2]).write_bytes(png)
