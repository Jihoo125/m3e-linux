#!/usr/bin/env python3
from __future__ import annotations
import argparse
from pathlib import Path
import subprocess

SEEDS = [0xFF000000, 0xFFFFFFFF, 0xFF6750A4, 0xFF0000FF, 0xFFFF0000, 0xFF00FF00,
         0xFFFFC107, 0xFF4C662B, 0xFF00639A, 0xFF7F7F7F, 0xFF123456, 0xFFFFD8E4]


def lower_first(name):
    return name[0].lower() + name[1:]


def parse_reference(text):
    hct = None
    roles = {}
    for line in text.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[0].replace('.', '').replace('-', '').isdigit():
            hct = tuple(map(float, fields))
        if len(fields) == 2:
            roles[lower_first(fields[0])] = int(fields[1], 16)
    return hct, roles


def parse_ours(text):
    hct = None
    roles = {}
    for line in text.splitlines():
        if line.startswith('HCT '):
            hct = tuple(map(float, line.split()[1:]))
        elif line.startswith('role '):
            fields = line.split()
            roles[fields[1]] = int(fields[2], 16)
        elif line.count(' #') == 1:
            role, color = line.split(' #')
            roles[role] = int(color, 16)
    return hct, roles


def run_matched(reference, source, seed, variant, dark):
    expected = subprocess.check_output([reference, f'{seed:08X}', str(variant), 'dark' if dark else 'light'], text=True)
    actual = subprocess.check_output([source, f'{seed:08X}', 'dark' if dark else 'light', str(variant)], text=True)
    rh, expected = parse_reference(expected)
    oh, actual = parse_ours(actual)
    failures = []
    for a, b, name in zip(rh, oh, ('hue', 'chroma', 'tone')):
        if abs(a - b) > 0.001:
            failures.append(f'{seed:08X} v{variant} {dark} {name}: {b} != {a}')
    for role, expected_color in expected.items():
        if actual.get(role) != expected_color:
            failures.append(f'{seed:08X} v{variant} {dark} {role}: #{actual.get(role, 0):08X} != #{expected_color:08X}')
    return failures, len(expected)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--reference', default='build/reference/mcu-reference')
    parser.add_argument('--ours', default='build/m3/m3-dump')
    parser.add_argument('--seeds', nargs='*', default=[f'{x:08X}' for x in SEEDS])
    args = parser.parse_args()
    errors = []
    count = 0
    for text in args.seeds:
        seed = int(text, 16) if text.startswith('FF') or any(c in 'ABCDEF' for c in text) else int(text)
        for variant in range(9):
            for dark in [False, True]:
                found, n = run_matched(args.reference, args.ours, seed, variant, dark)
                errors += found
                count += n
    print(f'Compared {count} reference roles: {len(errors)} mismatches')
    for line in errors[:60]:
        print(line)
    raise SystemExit(int(bool(errors)))
