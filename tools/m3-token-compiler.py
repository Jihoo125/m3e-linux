#!/usr/bin/env python3
"""Compile resolved Material 3 schemes into native toolkit artifacts.

The compiler deliberately does not derive colours: libm3 supplies a resolved
scheme, while this program validates it and emits byte-for-byte reproducible
artifacts using only the Python standard library.
"""

from __future__ import print_function

import argparse
import json
import os
from pathlib import Path
import re
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TOKENS = ROOT / "design" / "tokens"
DEFAULT_OUT = ROOT / "generated"
TOKEN_FILES = (
    "colour-roles.json",
    "typography.json",
    "shape.json",
    "motion.json",
    "elevation.json",
    "density.json",
    "iconography.json",
)
COLOR_RE = re.compile(r"^#[0-9a-fA-F]{6}$")


class CompilerError(Exception):
    """An input, validation, or output error suitable for a CLI diagnostic."""


def read_json(path):
    try:
        with path.open("r", encoding="utf-8") as stream:
            value = json.load(stream)
    except FileNotFoundError:
        raise CompilerError("missing file: {0}".format(path))
    except OSError as error:
        raise CompilerError("cannot read {0}: {1}".format(path, error))
    except json.JSONDecodeError as error:
        raise CompilerError(
            "invalid JSON in {0}:{1}:{2}: {3}".format(
                path, error.lineno, error.colno, error.msg
            )
        )
    if not isinstance(value, dict):
        raise CompilerError("{0}: top-level JSON value must be an object".format(path))
    return value


def load_tokens(directory):
    tokens = {}
    for filename in TOKEN_FILES:
        tokens[filename] = read_json(directory / filename)
    colour = tokens["colour-roles.json"]
    roles = colour.get("roles")
    required = colour.get("requiredRoles")
    palettes = colour.get("palettes")
    pairs = colour.get("contrastPairs")
    if not isinstance(roles, dict) or not roles:
        raise CompilerError("colour-roles.json: roles must be a non-empty object")
    if not isinstance(required, list) or not required:
        raise CompilerError("colour-roles.json: requiredRoles must be a non-empty list")
    if len(set(required)) != len(required) or not all(
        isinstance(role, str) for role in required
    ):
        raise CompilerError("colour-roles.json: requiredRoles contains invalid entries")
    unknown = sorted(set(required) - set(roles))
    if unknown:
        raise CompilerError(
            "colour-roles.json: required roles have no definitions: {0}".format(
                ", ".join(unknown)
            )
        )
    if not isinstance(palettes, dict) or not palettes:
        raise CompilerError("colour-roles.json: palettes must be a non-empty object")
    for palette_name, palette in palettes.items():
        if not isinstance(palette, dict) or not palette:
            raise CompilerError(
                "colour-roles.json: palette {0} must be a non-empty object".format(
                    palette_name
                )
            )
        for tone, value in palette.items():
            if not isinstance(tone, str) or not tone.isdigit() or not 0 <= int(tone) <= 100:
                raise CompilerError(
                    "colour-roles.json: palette {0} has invalid tone {1!r}".format(
                        palette_name, tone
                    )
                )
            validate_color(
                value, "colour-roles.json palette {0} tone {1}".format(palette_name, tone)
            )
    for role, definition in roles.items():
        if not isinstance(definition, dict):
            raise CompilerError("colour role {0}: definition must be an object".format(role))
        for mode in ("light", "dark"):
            reference = definition.get(mode)
            if not isinstance(reference, str) or reference.count(".") != 1:
                raise CompilerError(
                    "colour role {0}: invalid {1} palette reference".format(role, mode)
                )
            palette, tone = reference.split(".")
            value = palettes.get(palette, {}).get(tone) if isinstance(
                palettes.get(palette), dict
            ) else None
            validate_color(
                value, "colour role {0} {1} reference {2}".format(role, mode, reference)
            )
    if not isinstance(pairs, list) or not pairs:
        raise CompilerError("colour-roles.json: contrastPairs must be a non-empty list")
    for pair in pairs:
        if not isinstance(pair, dict):
            raise CompilerError("contrastPairs: every pair must be an object")
        for key in ("foreground", "background"):
            if pair.get(key) not in roles:
                raise CompilerError(
                    "contrastPairs: unknown {0} role {1!r}".format(key, pair.get(key))
                )
        minimum = pair.get("minimum")
        if isinstance(minimum, bool) or not isinstance(minimum, (int, float)) or minimum <= 1:
            raise CompilerError("contrastPairs: minimum must be a number greater than 1")
        if not isinstance(pair.get("required"), bool):
            raise CompilerError("contrastPairs: required must be a boolean")
    return tokens


def validate_color(value, label):
    if not isinstance(value, str) or not COLOR_RE.fullmatch(value):
        raise CompilerError("{0}: expected #RRGGBB, got {1!r}".format(label, value))
    return value.upper()


def normalize_scheme(scheme, colour_tokens):
    if not isinstance(scheme, dict):
        raise CompilerError("scheme: top-level JSON value must be an object")
    mode = scheme.get("mode")
    if mode not in ("light", "dark"):
        raise CompilerError("scheme.mode: expected 'light' or 'dark', got {0!r}".format(mode))
    source_color = validate_color(scheme.get("sourceColor"), "scheme.sourceColor")
    roles = scheme.get("roles")
    if not isinstance(roles, dict):
        raise CompilerError("scheme.roles: expected an object of role/colour pairs")
    definitions = colour_tokens["roles"]
    required = colour_tokens["requiredRoles"]
    missing = sorted(set(required) - set(roles))
    if missing:
        raise CompilerError("scheme.roles: missing required roles: {0}".format(", ".join(missing)))
    unknown = sorted(set(roles) - set(definitions))
    if unknown:
        raise CompilerError("scheme.roles: roles absent from colour-roles.json: {0}".format(
            ", ".join(unknown)
        ))
    normalized_roles = {}
    for role in definitions:
        if role in roles:
            normalized_roles[role] = validate_color(
                roles[role], "scheme.roles.{0}".format(role)
            )
    return {
        "formatVersion": 1,
        "mode": mode,
        "sourceColor": source_color,
        "roles": normalized_roles,
    }


def resolve_baseline_scheme(mode, colour_tokens):
    roles = {}
    for role, definition in colour_tokens["roles"].items():
        palette, tone = definition[mode].split(".")
        roles[role] = validate_color(
            colour_tokens["palettes"][palette][tone],
            "baseline {0} role {1}".format(mode, role),
        )
    return {
        "mode": mode,
        "sourceColor": "#6750A4",
        "roles": roles,
    }


def linear_channel(channel):
    value = channel / 255.0
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def relative_luminance(color):
    value = validate_color(color, "contrast colour")
    red, green, blue = (int(value[index:index + 2], 16) for index in (1, 3, 5))
    return (
        0.2126 * linear_channel(red)
        + 0.7152 * linear_channel(green)
        + 0.0722 * linear_channel(blue)
    )


def contrast_ratio(first, second):
    light = max(relative_luminance(first), relative_luminance(second))
    dark = min(relative_luminance(first), relative_luminance(second))
    return (light + 0.05) / (dark + 0.05)


def audit_scheme(scheme, colour_tokens):
    results = []
    for pair in colour_tokens["contrastPairs"]:
        foreground = pair["foreground"]
        background = pair["background"]
        ratio = contrast_ratio(
            scheme["roles"][foreground], scheme["roles"][background]
        )
        results.append({
            "foreground": foreground,
            "background": background,
            "minimum": float(pair["minimum"]),
            "required": pair["required"],
            "ratio": ratio,
            "passes": ratio + 1e-12 >= float(pair["minimum"]),
        })
    return results


def css_name(role):
    return re.sub(r"(?<!^)(?=[A-Z])", "-", role).lower()


def render_normalized(scheme):
    return json.dumps(scheme, indent=2, ensure_ascii=False) + "\n"


def render_qml(scheme):
    lines = [
        "// Generated by tools/m3-token-compiler.py; do not edit.",
        "pragma Singleton",
        "import QtQml 2.15",
        "",
        "QtObject {",
        "    readonly property bool isDark: {0}".format(
            "true" if scheme["mode"] == "dark" else "false"
        ),
        "    readonly property string sourceColor: \"{0}\"".format(scheme["sourceColor"]),
    ]
    for role, color in scheme["roles"].items():
        lines.append("    readonly property color {0}: \"{1}\"".format(role, color))
    lines.append("}")
    return "\n".join(lines) + "\n"


def render_gtk(scheme):
    lines = ["/* Generated by tools/m3-token-compiler.py; do not edit. */"]
    for role, color in scheme["roles"].items():
        lines.append("@define-color {0} {1};".format(role, color))
    aliases = {
        "theme_bg_color": "surface",
        "theme_fg_color": "onSurface",
        "theme_base_color": "surfaceContainerLowest",
        "theme_text_color": "onSurface",
        "theme_selected_bg_color": "primary",
        "theme_selected_fg_color": "onPrimary",
        "insensitive_bg_color": "surfaceContainer",
        "insensitive_fg_color": "onSurfaceVariant",
        "borders": "outlineVariant",
        "unfocused_borders": "outlineVariant",
        "warning_color": "tertiary",
        "error_color": "error",
        "success_color": "secondary",
    }
    roles = scheme["roles"]
    lines.extend(["", "/* Standard GTK aliases, also derived from the scheme. */"])
    for alias, role in aliases.items():
        lines.append("@define-color {0} {1};".format(alias, roles[role]))
    return "\n".join(lines) + "\n"


def render_xrdb(scheme):
    roles = scheme["roles"]
    lines = ["! Generated by tools/m3-token-compiler.py; do not edit."]
    for role, color in roles.items():
        lines.append("*m3.{0}: {1}".format(role, color))
    lines.extend([
        "",
        "*background: {0}".format(roles["surface"]),
        "*foreground: {0}".format(roles["onSurface"]),
        "*cursorColor: {0}".format(roles["primary"]),
        "*highlightColor: {0}".format(roles["primary"]),
        "*highlightTextColor: {0}".format(roles["onPrimary"]),
        "*borderColor: {0}".format(roles["outlineVariant"]),
    ])
    return "\n".join(lines) + "\n"


def render_qt_palette(scheme):
    roles = scheme["roles"]
    active = {
        "Window": "surface",
        "WindowText": "onSurface",
        "Base": "surfaceContainerLowest",
        "AlternateBase": "surfaceContainerLow",
        "ToolTipBase": "inverseSurface",
        "ToolTipText": "inverseOnSurface",
        "Text": "onSurface",
        "Button": "surfaceContainer",
        "ButtonText": "onSurface",
        "BrightText": "onPrimary",
        "Highlight": "primary",
        "HighlightedText": "onPrimary",
        "Link": "primary",
        "LinkVisited": "tertiary",
        "PlaceholderText": "onSurfaceVariant",
    }
    lines = [
        "; Generated by tools/m3-token-compiler.py; do not edit.",
        "[Palette]",
    ]
    for name, role in active.items():
        lines.append("{0}={1}".format(name, roles[role]))
    lines.extend([
        "",
        "[Disabled]",
        "Window={0}".format(roles["surface"]),
        "Base={0}".format(roles["surfaceContainerLowest"]),
        "Button={0}".format(roles["surfaceContainer"]),
        "WindowText={0}".format(roles["onSurfaceVariant"]),
        "Text={0}".format(roles["onSurfaceVariant"]),
        "ButtonText={0}".format(roles["onSurfaceVariant"]),
        "Highlight={0}".format(roles["surfaceContainerHighest"]),
        "HighlightedText={0}".format(roles["onSurfaceVariant"]),
        "",
        "[M3]",
    ])
    for role, color in roles.items():
        lines.append("{0}={1}".format(role, color))
    return "\n".join(lines) + "\n"


def render_css(scheme):
    lines = [
        "/* Generated by tools/m3-token-compiler.py; do not edit. */",
        ":root {",
        "  color-scheme: {0};".format(scheme["mode"]),
    ]
    for role, color in scheme["roles"].items():
        lines.append("  --m3-{0}: {1};".format(css_name(role), color))
    lines.append("}")
    return "\n".join(lines) + "\n"


def compile_artifacts(scheme, tokens):
    colour_tokens = tokens["colour-roles.json"]
    normalized = normalize_scheme(scheme, colour_tokens)
    contrast = audit_scheme(normalized, colour_tokens)
    failures = [result for result in contrast if result["required"] and not result["passes"]]
    if failures:
        details = "; ".join(
            "{0}/{1}={2:.2f}:1 (minimum {3:.2f}:1)".format(
                result["foreground"], result["background"], result["ratio"], result["minimum"]
            )
            for result in failures
        )
        raise CompilerError("required contrast checks failed: " + details)
    return {
        "scheme-normalized.json": render_normalized(normalized).encode("utf-8"),
        "theme.qml": render_qml(normalized).encode("utf-8"),
        "gtk.css": render_gtk(normalized).encode("utf-8"),
        "xrdb": render_xrdb(normalized).encode("utf-8"),
        "qt-palette.conf": render_qt_palette(normalized).encode("utf-8"),
        "colors.css": render_css(normalized).encode("utf-8"),
    }, contrast


def atomic_write(path, content):
    """Write one complete artifact in its destination directory, then rename."""
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        descriptor, temporary_name = tempfile.mkstemp(
            prefix="." + path.name + ".", suffix=".tmp", dir=str(path.parent)
        )
        try:
            with os.fdopen(descriptor, "wb") as stream:
                stream.write(content)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary_name, str(path))
        except BaseException:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass
            raise
    except OSError as error:
        raise CompilerError("cannot write {0}: {1}".format(path, error))


def check_artifacts(directory, artifacts):
    stale = []
    for name, expected in artifacts.items():
        path = directory / name
        try:
            actual = path.read_bytes()
        except FileNotFoundError:
            stale.append((path, "missing"))
            continue
        except OSError as error:
            raise CompilerError("cannot read {0}: {1}".format(path, error))
        if actual != expected:
            stale.append((path, "out of date"))
    return stale


def build_parser():
    parser = argparse.ArgumentParser(
        description="Validate and compile Material 3 design tokens without dependencies."
    )
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--scheme", type=Path, help="resolved scheme JSON to compile")
    selection.add_argument(
        "--all", action="store_true", help="compile built-in light and dark baseline schemes"
    )
    parser.add_argument(
        "--tokens", type=Path, default=DEFAULT_TOKENS,
        help="token directory (default: %(default)s)",
    )
    parser.add_argument(
        "--out", type=Path, default=DEFAULT_OUT,
        help="output directory; --all adds light/ and dark/ (default: %(default)s)",
    )
    parser.add_argument(
        "--check", action="store_true",
        help="fail if generated files are missing or differ; never write files",
    )
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    try:
        tokens = load_tokens(args.tokens)
        if args.all:
            jobs = [
                (args.out / mode, resolve_baseline_scheme(mode, tokens["colour-roles.json"]))
                for mode in ("light", "dark")
            ]
        else:
            jobs = [(args.out, read_json(args.scheme))]
        compiled = [compile_artifacts(scheme, tokens) for _, scheme in jobs]
        failed = False
        for (directory, _), (artifacts, contrast) in zip(jobs, compiled):
            for result in contrast:
                if not result["required"] and not result["passes"]:
                    print(
                        "warning: optional contrast {0}/{1}={2:.2f}:1 "
                        "(minimum {3:.2f}:1)".format(
                            result["foreground"], result["background"],
                            result["ratio"], result["minimum"],
                        ),
                        file=sys.stderr,
                    )
            if args.check:
                stale = check_artifacts(directory, artifacts)
                for path, reason in stale:
                    print("STALE {0}: {1}".format(path, reason))
                if stale:
                    failed = True
                else:
                    print("OK {0}: {1} artifacts are current".format(
                        directory, len(artifacts)
                    ))
            else:
                for name, content in artifacts.items():
                    atomic_write(directory / name, content)
                print("WROTE {0}: {1} artifacts".format(directory, len(artifacts)))
        return 1 if failed else 0
    except CompilerError as error:
        print("m3-token-compiler: error: {0}".format(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
