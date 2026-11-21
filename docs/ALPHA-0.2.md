# Alpha 0.2.0 release checklist

**Released:** 2026-11-21 · supersedes Alpha 0.1 (`ALPHA-0.1.md`).

Alpha 0.2.0 = the verified Alpha 0.1 base **plus** the Material 3 Expressive
layer, four new real applications, Firefox ESR with generated Material chrome,
XDG autostart, and compositor launch hotkeys. Every box below has evidence.

## Expressive

- [x] Spring-physics motion tokens: `M3Theme.spring()` exposes under-damped
      spatial and critically damped effects springs (unit test `m3theme`).
- [x] Press shape morphing on buttons and spring-driven switch thumbs.
- [x] Emphasized variable-weight typography (`fontEmphasized`, `M3Label.emphasized`).
- [x] Expressive color variant selectable in Settings → Appearance → Color
      style; applied live through `matsessiond` `SetVariant`.

## Applications (all real backends)

- [x] Terminal `matterminal` — forkpty PTY + VT/ANSI emulator; unit tests.
- [x] Image viewer `matview` — directory browsing, metadata, navigation.
- [x] Screenshot `matshot` — wlr-screencopy-unstable-v1 client; the compositor
      implements the screencopy manager.
- [x] Calendar `matcal` — persistent JSON events; unit tests.
- [x] Browser — Firefox ESR in the image; `matbrowser` wrapper applies a
      `userChrome.css` generated from the live wallpaper scheme (native
      Wayland, dedicated profile).
- [x] All nine apps smoke-tested as real Wayland clients
      (`build/test-new-apps.sh`).

## Shell / session

- [x] XDG autostart (honors OnlyShowIn/NotShowIn/Hidden).
- [x] Ctrl+Alt+T terminal and Ctrl+Alt+B browser compositor hotkeys.
- [x] In-guest watchdog reports session process state on serial.

## Acceptance (2026-11-21, on the released ISO)

- [x] UEFI boot → exposed QML shell → terminal + Firefox running:
      `tests/vm/run-qemu.sh --mode uefi`, `build/vm/apps-uefi.png`.
- [x] BIOS boot → same: `--mode bios`, `build/vm/apps-bios.png`.
- [x] Local suites: unified build, installer ctest 3/3, compositor self-test,
      Wayland client, 13 token tests, calc/editor/terminal/calendar tests,
      `m3theme` Expressive test.
- [x] ISO SHA-256:
      `c7956d6895a15d6449f14e6e73b22b0c7549f31e54400458ad087b518b10a574`
      (`build/iso/materialized-0.2.0-amd64.iso.sha256`).
- [x] Package manifest recorded (`materialized-0.2.0-amd64.iso.packages`).

## Not part of Alpha 0.2.0

Music player, PackageKit software center, full Settings coverage, lock
screen/PAM, graphical installer wizard, XWayland, gamma-control, workspace
overview, and real-hardware/accessibility sign-off. See `ROADMAP.md`.

## Release process

Every new version is committed and pushed to
https://github.com/Jihoo125/m3e-linux.git as
`Qwen4-Max wooo <jihoo125@j1t.xyz>` via `build/release.sh`.
