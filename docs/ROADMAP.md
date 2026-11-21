# Materialized Linux — Roadmap

**Updated:** 2026-11-21. The project targets Debian 13 (trixie), x86_64, Wayland,
systemd, GRUB, APT, and Material 3. This file separates the small, verified Alpha
0.1 base from the complete distribution vision.

## Alpha 0.1 — runnable desktop base

| Area | State | Evidence / command |
|---|---|---|
| CAM16/HCT and tonal palettes | Implemented | `libs/libm3`, wallpaper-derived schemes |
| Token compiler and accessibility | Passing | `python3 -m unittest tests.unit.token_compiler_test -v` (13 tests) |
| QML theme/components | Implemented | `build/test-theme.sh`, `build/screenshot-shell.sh` |
| wlroots compositor | Headless and Qt client verified | `build/test-compositor.sh`, `build/test-wayland-client.sh` |
| Session theme service | Implemented | `matsessiond` writes scheme artifacts and D-Bus theme API |
| Shell | Panel, launcher/search, notifications, quick settings | `build/screenshot-shell.sh` |
| Core applications | Calculator, editor, monitor, files, Settings | `build/build-core-apps.sh` |
| Installer engine | Safe planning and real subprocess/cleanup tests | `bash build/test-installer.sh` |
| Live ISO | Built and hashed | `sudo bash platform/iso/build.sh` |
| UEFI boot to QML session | Passed 2026-11-19 | `tests/vm/run-qemu.sh --mode uefi`, `build/vm/boot-uefi.png` |
| BIOS boot to QML session | Passed 2026-11-19 | `tests/vm/run-qemu.sh --mode bios`, `build/vm/boot-bios.png` |
| VM desktop screenshot | Captured and inspected | Panel, launcher, clock and shortcuts render in both modes |

Alpha 0.1 is **complete as of 2026-11-19**: the ISO boots to the QML desktop in
both UEFI and BIOS QEMU, the shell is visibly exposed, and screenshots were
inspected. ISO SHA-256:
`8986f897d2918ebbaa018ed595ebaa03c43c44b9eaf8565ce08898846727ccfe`.
The smaller scope is intentional: it proves that the real desktop works before
adding the rest of the distribution.

## Expressive + applications milestone — released as Alpha 0.2.0 (2026-11-21)

| Area | State | Evidence / command |
|---|---|---|
| M3 Expressive motion physics | Implemented + unit-tested | `M3Theme.spring()` (spatial under-damped, effects critical), `ctest --test-dir build/qml` (`m3theme`) |
| Expressive shape morph + emphasized type | Implemented | M3Button/M3Switch spring morphs, `M3Theme.fontEmphasized()`, `M3Label.emphasized` |
| Expressive color variant end-to-end | Implemented | Settings → Appearance → Color style; `matsessiond` `SetVariant`; scheme JSON `variant` |
| Terminal (real PTY + VT emulator) | Implemented + tests | `apps/matterminal`, `ctest` matterminal; runs in VM via Ctrl+Alt+T |
| Image viewer | Implemented | `apps/matview`; Wayland client smoke test |
| Screenshot tool | Implemented | `apps/matshot` + `wlr_screencopy_manager_v1` in matcomp |
| Calendar with persistent events | Implemented + tests | `apps/matcal`, `ctest` matcal (JSON persistence) |
| Firefox ESR browser + Material chrome | Implemented, runs in VM | `matbrowser` wrapper, `firefox-userChrome.css` artifact; guest watchdog shows `firefox-esr` pid |
| XDG autostart + launch hotkeys | Implemented | `ApplicationModel::launchAutostart()` (mechanism); Ctrl+Alt+T / Ctrl+Alt+B; 0.2.0 demo entries removed in 0.2.1 |
| UEFI/BIOS acceptance with typed input + apps | Passed 2026-11-21 (0.2.1) | `build/vm/input-uefi.png`, `input-bios.png` (typed command executed), `apps-uefi.png`, `apps-bios.png` |

ISO SHA-256 for Alpha 0.2.0 (superseded):
`c7956d6895a15d6449f14e6e73b22b0c7549f31e54400458ad087b518b10a574`.
Every release is committed and pushed to
https://github.com/Jihoo125/m3e-linux.git as `Qwen4-Max wooo <jihoo125@j1t.xyz>`
via `build/release.sh`.

## Input & cursor correctness — released as Alpha 0.2.1 (2026-11-21)

Fixes for the VMware Workstation Pro 25H2 report (no cursor, dead keyboard,
unwanted auto-opened apps):

- The libinput backend was never created (`WLR_BACKENDS` overwritten to `drm`
  only); now `drm,libinput`, with per-device serial logging.
- Newly mapped windows take keyboard focus (`View::on_map` → `focus_view`);
  previously hotkey/launcher apps received no keystrokes at all.
- Software cursors are locked per output so the pointer is composited into the
  frame on every virtual GPU; xcursor theme load failures are logged.
- The 0.2.0 demo autostart (terminal + Firefox) is removed; apps open only on
  user action (launcher, Ctrl+Alt+T / Ctrl+Alt+B).
- Acceptance types a real command into the terminal and gates on its serial
  output, in both UEFI and BIOS QEMU; VMware Workstation boot + input verified
  via `vmrun` + VNC remote display (`build/test-vmware.ps1`).

ISO SHA-256 for Alpha 0.2.1 (current):
`c5d2b42c33d477f3a6480614822d8b98aadb05869f5f062afad469ebc1d73886`.
Release checklist: `docs/ALPHA-0.2.1.md`.

## Next milestone — system integration

- Layer-shell placement for panel/overlays and full wallpaper service.
- Complete quick settings for NetworkManager, BlueZ, audio and display.
- Lock screen with PAM and `ext-session-lock`.
- [x] Screencopy support (wlr-screencopy-v1) and the screenshot application.
- Workspace overview, gamma-control/Night Light, XWayland, M3 window decorations.

## Application milestone

- [x] Terminal with a real PTY backend and VT emulator (`matterminal`).
- [ ] Software Center backed by PackageKit/AppStream.
- [x] Image viewer (`matview`), screenshot tool (`matshot`), calendar (`matcal`).
- [ ] Music player.
- [x] Firefox ESR packaging with generated Material chrome (`matbrowser`).
- [ ] Full Settings coverage for network, Bluetooth, power, storage, users and updates.

## Installation milestone

- QML installer wizard with real `matinstall-core` progress and error handling.
- Manual and automatic partitioning UI.
- Unattended install validation in UEFI and BIOS VMs.
- Dual-boot preservation and Btrfs snapshot integration.
- Signed, reproducible release image with package manifest and VM test matrix.

## Release quality milestone

- Accessibility audit, keyboard navigation, screen-reader support and OSK.
- Performance budgets on reference Intel/AMD integrated graphics.
- Real GPU/UEFI/BIOS hardware validation.
- User, administrator, contributor and theming guides.

## Completed decisions

C++20, Qt 6/QML, wlroots, Debian 13, Wayland-first, systemd, APT, GRUB hybrid
UEFI/BIOS, ext4 default and Btrfs option. See `DECISIONS.md` for the rationale
and `TODO.md` for the current short-term task list.
