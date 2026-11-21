#!/usr/bin/env python3
"""Fail when app/shell QML hard-codes a visual colour instead of a theme role.

Transparent/white icon masks and a few deliberate diagnostics are handled by
M3Theme and M3 components, not application pages. Add a narrowly-scoped
'mat-colour: allow' comment only for image pixel data, never normal UI colours.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

HEX = re.compile(r"(?<![\w#])#[0-9a-fA-F]{6}(?:[0-9a-fA-F]{2})?\b")
RGB = re.compile(r"\b(?:rgb|rgba)\s*\(", re.IGNORECASE)
LITERAL = re.compile(r'\b(?:color|textColor|containerColor|contentColor)\s*:\s*["\'](?!transparent["\'])')
SKIP_PARTS = {"build", ".git", "generated", "third_party", "node_modules"}


def inspect(path: pathlib.Path) -> list[str]:
    errors: list[str] = []
    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.split("//", 1)[0]
        if "mat-colour: allow" in raw:
            continue
        if HEX.search(line) or RGB.search(line) or LITERAL.search(line):
            errors.append(f"{path}:{lineno}: hard-coded colour; use M3Theme.color(role): {raw.strip()}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("roots", nargs="*", type=pathlib.Path, default=[pathlib.Path("apps"), pathlib.Path("shell")])
    args = parser.parse_args()
    errors: list[str] = []
    for root in args.roots:
        if not root.exists():
            continue
        for path in root.rglob("*.qml"):
            if any(part in SKIP_PARTS for part in path.parts):
                continue
            errors.extend(inspect(path))
    if errors:
        print("\n".join(errors), file=sys.stderr)
        print(f"\n{len(errors)} hard-coded colour(s). Use semantic Material roles instead.", file=sys.stderr)
        return 1
    print("Material colour lint: no hard-coded application colours found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
