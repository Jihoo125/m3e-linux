# M3 token compiler contract

This specification defines the boundary between the numerical colour engine and
native toolkit outputs. It applies to `../tokens/*.json` and
`../../tools/m3-token-compiler.py`.

## Pipeline

1. `libm3` uses the source colour and user preferences to derive **both** modes.
2. The producer supplies `{mode, sourceColor, roles}` for one resolved scheme.
3. The compiler loads the token directory and validates every complete required
   role before producing any file. A scheme role must also be declared in
   `colour-roles.json`; silently ignoring misspellings would cause drift.
4. The compiler checks all required foreground/background pairs using WCAG 2
   sRGB relative luminance: linearize each 8-bit channel, compute
   `0.2126 R + 0.7152 G + 0.0722 B`, then divide the lighter luminance plus
   0.05 by the darker luminance plus 0.05.
5. The requested output directory receives canonical JSON, QML, GTK CSS, xrdb,
   Qt palette, and CSS custom properties. Each file is replaced atomically. The
   six-file set is not a transactional filesystem bundle.

The compiler does **not** perform HCT/CAM16 derivation and never substitutes
missing or unreadable roles. Dynamic variants and contrast levels are handled
upstream. Built-in `--all` output is the reference tonal-spot baseline, not a
replacement for runtime dynamic schemes.

## Consumer conventions

| Artifact | Contract |
|---|---|
| `scheme-normalized.json` | `formatVersion = 1`, normalized mode/source, role object in source-table order |
| `theme.qml` | QML singleton, `QtObject`, `isDark`, `sourceColor`, and `readonly property color` for each role |
| `gtk.css` | `@define-color role #RRGGBB` and derived standard GTK aliases; toolkit rules remain component code |
| `xrdb` | `*m3.role` entries plus generic background, foreground, cursor, and selection resources |
| `qt-palette.conf` | Key/value palette for widget fallback; full role set retained in `[M3]` |
| `colors.css` | `--m3-role-name` under `:root`; explicit light/dark `color-scheme` |

Opaque scheme colours are uppercase `#RRGGBB`. Opacity is applied by state-layer,
elevation, scrim, or renderer tokens. There are no timestamps or absolute paths
in artifacts, making two compilations of identical inputs byte-identical.
JSON is UTF-8 with two-space indentation and a final newline; generated text
also uses LF on every host.

`--check` performs the same parsing, normalization, and contrast validation as
generation, then compares bytes without changing the output directory. Missing
or stale artifacts return status 1. Invalid input or required contrast failures
return status 2 and produce no artifacts. Optional 3:1 audits warn during a
normal compile. The standalone auditor's `--strict` elevates them to failures.

## Token ownership and accessibility

Typography uses Inter and the complete display/headline/title/body/label M3
scale. Values are logical `sp`, not assumed physical pixels. A renderer converts
shape `dp`, type `sp`, and motion `ms` using its platform scale. Full corners
resolve to half the shorter side; the stored `-1` is only a sentinel.

Reduced motion overrides animation duration without removing essential state
feedback. Compact density may shrink the visual control but not the touch
target. Icon state layers use the current foreground role and declared opacity;
they do not introduce new colours.

To add a role, add its definition and palette references to `requiredRoles` and
`roles`, provide any necessary baseline palette tone, add its semantic contrast
pair, and update all producers. To add a variant, add its exact name to the
allow-list. Do not create component-specific colour constants: add a semantic
role or a non-colour token instead.
