# Alpha 0.1 release checklist

This checklist is intentionally smaller than the full Materialized Linux roadmap.
An item is checked only when there is a corresponding implementation and test.

## Foundation

- [x] C++20 Material 3 color engine with CAM16/HCT, palettes, dynamic schemes,
      wallpaper quantization and seed selection.
- [x] QML Material components and live `M3Theme` scheme loading.
- [x] Deterministic token compiler and accessibility checks.
- [x] wlroots compositor with headless and hardware backends.
- [x] Real Qt/Wayland client smoke test against `matcomp`.
- [x] Wallpaper-derived light/dark palette generation and QML reload.
- [x] Shell launcher/search, panel, notifications and quick settings.
- [x] Calculator and text-editor unit tests.
- [x] Installer geometry, command-generation, subprocess and cleanup tests.
- [x] Python token compiler tests (13 tests).
- [x] Debian live ISO script with hybrid UEFI/BIOS GRUB.
- [x] QEMU boot-to-live-session smoke test passes in UEFI mode (2026-11-19,
      visible+exposed QML shell, screenshot `build/vm/boot-uefi.png`).
- [x] QEMU boot-to-live-session smoke test passes in BIOS mode (2026-11-19,
      screenshot `build/vm/boot-bios.png`).
- [x] ISO SHA-256 and package manifest recorded:
      `8986f897d2918ebbaa018ed595ebaa03c43c44b9eaf8565ce08898846727ccfe`
      (`build/iso/materialized-0.1.0-amd64.iso.sha256`,
      `build/iso/materialized-0.1.0-amd64.iso.packages`).
- [x] Screenshot of the live desktop captured in the VM and inspected:
      panel, launcher, workspace indicator, clock and app shortcuts render
      from the live QML shell in both firmware modes.

## Explicitly not part of Alpha 0.1 completion

- The music player and the PackageKit/AppStream software center.
- Full NetworkManager/BlueZ/UPower/UDisks2/PackageKit settings coverage.
- Lock screen and PAM integration.
- Full graphical installer wizard (the safe installer core is separate).
- XWayland, gamma-control, workspace overview, gestures, OSD,
  system tray, and complete server-side Material decorations.
- Production-ready accessibility, performance budgets, and real-hardware support.
- Any claim that the complete Materialized Linux distribution is finished.

## Post-alpha milestone (2026-11-21, verified)

Added after the Alpha 0.1 acceptance and re-verified in both firmware modes:

- [x] Material 3 Expressive: spring-physics motion tokens, press shape morphing,
      emphasized variable-weight type, Expressive variant in Settings
      (`m3theme` unit test; `M3Theme.spring()` / `fontEmphasized()`).
- [x] Terminal (`matterminal`, real PTY + VT emulator), image viewer (`matview`),
      screenshot tool (`matshot` over wlr-screencopy), calendar (`matcal` with
      persistent JSON events) — all unit-tested where logic exists and smoke-
      tested as real Wayland clients (`build/test-new-apps.sh`).
- [x] Firefox ESR in the image, launched by `matbrowser` with a Material 3
      `userChrome.css` generated from the live scheme; native Wayland.
- [x] XDG autostart in the shell and Ctrl+Alt+T / Ctrl+Alt+B hotkeys; the VM
      acceptance captures show the terminal and Firefox running
      (`build/vm/apps-uefi.png`, `build/vm/apps-bios.png`), confirmed by an
      in-guest watchdog process list.

## Test commands

```bash
bash build/test-compositor.sh
bash build/test-wayland-client.sh
bash build/test-theme.sh
bash build/screenshot-shell.sh
bash build/test-installer.sh
python3 -m unittest tests.unit.token_compiler_test -v

sudo bash platform/iso/build.sh
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.1.0-amd64.iso --mode uefi
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.1.0-amd64.iso --mode bios
```

A release candidate must not be described as Alpha 0.1 complete while the VM
checks or the ISO manifest are missing. See `docs/ROADMAP.md` for the full plan.
