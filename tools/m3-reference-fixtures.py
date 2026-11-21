#!/usr/bin/env python3
"""Generate pinned fixtures using the actual material-color-utilities wheel API."""
from __future__ import annotations
import argparse
import json
import re
import subprocess
from pathlib import Path
import material_color_utilities as m

SEEDS = [0xFF000000, 0xFFFFFFFF, 0xFF6750A4, 0xFF0000FF, 0xFFFF0000, 0xFF00FF00,
         0xFFFFC107, 0xFF4C662B, 0xFF00639A, 0xFF7F7F7F, 0xFF123456, 0xFFFFD8E4]
VARIANTS = {
    "MONOCHROME": 0, "NEUTRAL": 1, "TONALSPOT": 2, "VIBRANT": 3,
    "EXPRESSIVE": 4, "FIDELITY": 5, "CONTENT": 6, "RAINBOW": 7, "FRUITSALAD": 8,
}


def camel(name: str) -> str:
    return re.sub(r"_([a-z])", lambda match: match.group(1).upper(), name)


def generate(output: Path) -> None:
    result = {}
    for seed in SEEDS:
        h = m.Hct(seed)
        item = {"hct": {"hue": h.hue, "chroma": h.chroma, "tone": h.tone}, "schemes": {}}
        for variant_name, variant_id in VARIANTS.items():
            for dark in [False, True]:
                scheme = m.DynamicScheme(h, variant_name, dark, 0.0)
                roles = {}
                for name in dir(scheme):
                    if name.startswith("_") or name in {
                        "contrast_level", "is_dark", "source_color_hct", "variant",
                        "primary_palette", "secondary_palette", "tertiary_palette",
                        "neutral_palette", "neutral_variant_palette", "error_palette",
                        "dict",
                    }:
                        continue
                    value = getattr(scheme, name)
                    if isinstance(value, int) and not isinstance(value, bool):
                        roles[camel(name)] = value
                item["schemes"][f"{variant_name}:{int(dark)}"] = roles
        result[f"{seed:08X}"] = item
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Generated {len(result)} upstream fixtures -> {output}")


def verify(dump: Path, fixtures: Path, tolerance: int) -> int:
    expected = json.loads(fixtures.read_text(encoding="utf-8"))
    failures = []
    total = 0
    for seed, item in expected.items():
        for key, roles in item["schemes"].items():
            variant, dark = key.split(":")
            actual = subprocess.run([str(dump), seed, "dark" if dark == "1" else "light", str(VARIANTS[variant])],
                                    check=True, text=True, capture_output=True).stdout
            found = {}
            for line in actual.splitlines():
                parts = line.split()
                if len(parts) == 3 and parts[1].startswith("#"):
                    found[parts[0]] = int(parts[1][1:], 16)
            for role, argb in roles.items():
                if role not in found:
                    failures.append(f"{seed} {key} {role}: role missing")
                    continue
                total += 1
                for shift in (16, 8, 0):
                    a = (argb >> shift) & 0xff
                    b = (found[role] >> shift) & 0xff
                    if abs(a - b) > tolerance:
                        failures.append(f"{seed} {key} {role}: #{argb:08X} != #{found[role]:08X}")
                        break
    print(f"Compared {total} roles; {len(failures)} mismatches")
    for failure in failures[:40]:
        print(failure)
    return int(bool(failures))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--generate", type=Path)
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--dump", type=Path)
    parser.add_argument("--fixtures", type=Path, default=Path("tests/unit/m3/fixtures.json"))
    parser.add_argument("--tolerance", type=int, default=1)
    args = parser.parse_args()
    if args.generate:
        generate(args.generate)
    if args.verify:
        if not args.dump:
            parser.error("--verify needs --dump")
        raise SystemExit(verify(args.dump, args.fixtures, args.tolerance))
