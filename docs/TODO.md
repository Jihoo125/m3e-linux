# Materialized Linux — current TODO

**Updated:** 2026-11-21 · the full roadmap remains in `ROADMAP.md`.

Legend: `[x]` implemented and verified · `[~]` in progress · `[ ]` not started.

## Verified foundations

- [x] Material 3 tokens and deterministic compiler (13 Python tests).
- [x] CAM16/HCT engine and wallpaper-derived light/dark schemes.
- [x] QML theme singleton and Material controls; gallery screenshot verified.
- [x] wlroots compositor, headless self-test and real Qt/Wayland client test.
- [x] Shell panel, launcher, notifications and quick-settings backends.
- [x] Calculator and text-editor unit tests.
- [x] File manager, system monitor and basic Settings backends.
- [x] Safe installer core and CLI safety tests.
- [x] ISO/session packaging and QEMU harness.

## Alpha 0.1 release blockers (all passed 2026-11-19)

- [x] Produce `build/iso/materialized-0.1.0-amd64.iso` with the actual session.
- [x] Boot the ISO in QEMU and capture the serial session marker (UEFI).
- [x] Boot the ISO in QEMU and capture the serial session marker (BIOS).
- [x] Capture a VM screenshot and inspect it for a usable desktop.
- [x] Save SHA-256 and the ISO package manifest.
- [x] Update `docs/ALPHA-0.1.md` when every release check passes.

## Expressive + applications milestone (passed 2026-11-21)

- [x] M3 Expressive spring motion, shape morph, emphasized type (`m3theme` test).
- [x] Expressive color variant selectable in Settings, live via D-Bus.
- [x] Terminal with real PTY + VT emulator (`matterminal`, unit tests).
- [x] Image viewer (`matview`), screenshot tool (`matshot`, wlr-screencopy).
- [x] Calendar with persistent events (`matcal`, unit tests).
- [x] Firefox ESR packaged; `matbrowser` wrapper with generated M3 chrome.
- [x] XDG autostart and Ctrl+Alt+T / Ctrl+Alt+B launch hotkeys.
- [x] UEFI + BIOS acceptance with terminal and Firefox running in the guest
      (watchdog process list + `build/vm/apps-*.png`).

## Remaining work

- [ ] Music player and PackageKit/AppStream software center.
- [ ] Complete NetworkManager, BlueZ, audio and display settings pages.
- [ ] Implement lock screen/PAM and proper layer-shell panel placement.
- [ ] Finish graphical installer UI and unattended VM installation matrix.
- [ ] Add XWayland, gamma-control and workspace overview.
- [ ] Validate accessibility, performance budgets and real GPU hardware.

## Environment

- [x] WSL2 development toolchain installed.
- [x] WSL clock repair and source-mtime repair scripts.
- [x] live-build/debootstrap/QEMU dependencies installed.
- [ ] Optional KVM acceleration (TCG fallback is supported but slower).

Do not treat the Alpha 0.1 checklist as the full distribution roadmap. The
remaining work is deliberately documented rather than represented as finished.
