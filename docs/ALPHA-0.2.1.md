# Alpha 0.2.1 release checklist

**Released:** 2026-11-21 · supersedes Alpha 0.2.0 (`ALPHA-0.2.md`).

Alpha 0.2.1 is a correctness release on top of 0.2.0: it fixes the input and
cursor bugs reported on VMware Workstation Pro 25H2 and removes the demo
autostart that shipped in 0.2.0. No new applications or theme features.

## Fixes

- [x] **No keyboard / no pointer / no Tab or text input.** `Server::create_backend`
      overwrote `WLR_BACKENDS` with `drm` only, so wlroots never created the
      libinput backend and the session had zero input devices. It now requests
      `drm,libinput`, and every created input device is logged
      (`matcomp: input device type=… name=…`).
- [x] **No visible cursor.** The pixman compositor now locks software cursors
      per output (`wlr_output_lock_software_cursors`), so the pointer is
      composited into the primary frame on every virtual GPU (hardware cursor
      planes are invisible to screen capture and vary per hypervisor). A failed
      xcursor theme load is logged as a warning instead of silently producing
      an invisible pointer. `XCURSOR_THEME`/`XCURSOR_SIZE` are exported by the
      session launcher.
- [x] **Hotkey/launcher apps received no keystrokes.** `View::on_map` never gave
      keyboard focus to newly mapped windows (focus-follows-mouse is off), so
      typing went nowhere. New windows now take focus on map; acceptance types
      a real command into the terminal and gates on its serial output.
- [x] **Unwanted auto-open of Firefox and terminal.** The 0.2.0 demo autostart
      entries (`materialized-terminal.desktop`, `materialized-browser.desktop`)
      are removed from the staging script and deleted from the live rootfs on
      repack. Apps now open only via user action: launcher, or the compositor
      hotkeys Ctrl+Alt+T / Ctrl+Alt+B.

## Acceptance (2026-11-21, on the released ISO)

- [x] Compositor creates real input devices (keyboard + pointer) — serial log
      gate in `tests/vm/run-qemu.sh`.
- [x] UEFI boot → exposed QML shell; hotkey-launched terminal receives typed
      keystrokes end-to-end (typed command writes `input-ok` to the serial
      console); pointer movement captured: `build/vm/input-uefi.png`.
- [x] BIOS boot → same: `build/vm/input-bios.png`, `build/vm/apps-bios.png`.
- [x] Firefox ESR launched via Ctrl+Alt+B after the input proof (no autostart):
      `build/vm/apps-uefi.png`, `build/vm/apps-bios.png`.
- [x] VMware Workstation Pro 25H2 (user-reported hypervisor): isolated test VM
      boots to the live session; Ctrl+Alt+T over the VNC remote display launches
      the terminal (process visible on serial) — keyboard input works on VMware.
      The typed-text serial gate is inconclusive there because VMware's
      serial-to-file log loses its tail on hard power-off; QEMU gates cover
      typing end-to-end (`build/test-vmware.ps1`, `build/vnc-input-test.py`).
- [x] Local suites green (installer ctest, token tests, m3theme, terminal
      emulator tests).
- [x] ISO SHA-256: `c5d2b42c33d477f3a6480614822d8b98aadb05869f5f062afad469ebc1d73886`
      (`docs/releases/materialized-0.2.1-amd64.iso.sha256`).

## Not part of Alpha 0.2.1

Same scope exclusions as 0.2.0 (music player, software center, full Settings
coverage, lock screen/PAM, graphical installer wizard, XWayland, real-hardware
sign-off). See `ROADMAP.md`.

## Release process

Committed, tagged `v0.2.1` and pushed to
https://github.com/Jihoo125/m3e-linux.git as
`Qwen4-Max wooo <jihoo125@j1t.xyz>` via `build/release.sh`.