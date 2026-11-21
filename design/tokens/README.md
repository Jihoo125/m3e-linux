# Design tokens

The JSON files in this directory are the design-system source of truth. `libm3`
resolves dynamic colour and passes a complete scheme to
`tools/m3-token-compiler.py`; applications must consume generated artifacts or
runtime scheme bindings rather than literal values.

## Colour

`colour-roles.json` includes every M3 role in the compatibility set and the
newer surface-container and fixed-colour roles. Each role has a light and dark
`palette.tone` reference. The embedded palettes provide deterministic baseline
schemes; a user-derived scheme may replace every role but may not omit or invent
roles without updating this file.

A resolved input scheme has exactly this core shape:

```json
{
  "mode": "light",
  "sourceColor": "#6750A4",
  "roles": {
    "primary": "#6750A4",
    "onPrimary": "#FFFFFF"
  }
}
```

The example is abbreviated: all `requiredRoles` must be present. Colours are
opaque sRGB `#RRGGBB` values. State, scrim, shadow, and disabled opacity belong
to non-colour tokens, not alpha-bearing scheme values. Dynamic derivation,
including contrast-level adjustments, is `libm3`'s responsibility.

`contrastPairs` declares normal-text pairs at 4.5:1 and supporting non-text
pairs at 3:1. Required text-pair failures block compilation. Optional failures
are warnings by default and errors under `a11y-contrast.py --strict`.

## Compile and verify

Run from the repository root with Python 3.8 or newer; no packages are needed:

```sh
python3 tools/m3-token-compiler.py --scheme scheme.json --out generated/
python3 tools/m3-token-compiler.py --all --out generated/
python3 tools/m3-token-compiler.py --all --out generated/ --check
python3 tools/a11y-contrast.py --scheme generated/dark/scheme-normalized.json --strict
```

A single scheme writes directly into `--out`. `--all` writes the two baseline
schemes into `light/` and `dark/` beneath it. Every directory contains:

- `scheme-normalized.json`: canonical mode, source colour, and ordered roles;
- `theme.qml`: a `pragma Singleton` `QtObject` with read-only colour properties;
- `gtk.css`: GTK named colours and standard compatibility aliases;
- `xrdb`: M3 resources plus conventional X11 colour resources;
- `qt-palette.conf`: Qt widget active/disabled groups and the full M3 role set;
- `colors.css`: kebab-case custom properties and the matching `color-scheme`.

Generation is independent of wall-clock time, locale, input key order, and host
path. Writes use a temporary file in the destination directory followed by an
atomic rename. `--check` only reads files and exits unsuccessfully when any
artifact is absent or differs. Generation does not delete unrelated files.

## Token families

- `typography.json`: the fifteen-role M3 scale in Inter, with Roboto/Noto
  fallbacks, weights, sp sizes, line heights, and tracking.
- `shape.json`: M3 corner scale and asymmetric families. `full = -1` is a
  sentinel for half the shorter side, never a negative rendering radius.
- `motion.json`: M3 cubic Bézier curves and duration scale. The web baseline
  uses `emphasized = standard`; component selection, not curve duplication,
  gives emphasized motion its meaning. Honour the reduced-motion override.
- `elevation.json`: M3 levels 0–5 and renderer shadow parameters.
- `density.json`: comfortable/compact control metrics, 4 dp spacing rhythm,
  adaptive breakpoints, and an invariant 48 dp touch target.
- `iconography.json`: the independently drawn 24 dp grid, exports, symbolic
  roles, and state-layer opacities. It does not embed third-party glyphs.

See `../specs/design-tokens.md` for consumer contracts and extension rules.
