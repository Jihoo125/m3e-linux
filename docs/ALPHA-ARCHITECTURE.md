# Architecture notes for Alpha 0.1

The original architecture document is retained in `ARCHITECTURE.md`. This short
note records the actual Alpha 0.1 implementation boundary.

## Verified desktop path

1. `matcomp` creates a wlroots Wayland server, scene graph, output, seat, xdg-shell
   and layer-shell surfaces. It is a real C++20 compositor, not a mock window.
2. `matsessiond` decodes a wallpaper with Qt, quantizes it with `libm3`, creates
   light/dark dynamic schemes, and atomically writes scheme JSON/QML/GTK/xrdb
   artifacts. Theme controls are exposed through `org.materialized.Theme1`.
3. `matshell` loads `M3Theme` and its QML controls, discovers real `.desktop`
   applications, serves notifications, and exposes quick settings backed by
   PipeWire and the kernel backlight interface.
4. The core applications use real system state: a safe arithmetic parser, an
   atomic text document writer, `/proc` process statistics, a Qt file model and
   D-Bus/system settings.
5. `matinstall-core` independently validates and plans an installation. Its
   destructive operation requires explicit opt-in, root and a real disk probe.
   The graphical wizard is not part of Alpha 0.1.
6. The live image is a real Debian rootfs assembled with live-build. The session
   launcher starts the compositor, session daemon and shell. The QML shell writes
   the serial acceptance marker after it has loaded and shown its window.

## Acceptance boundary

- The unified CMake build must succeed.
- Token, installer and core-app unit tests must pass.
- A Qt/Wayland client must run under the compositor.
- The ISO must boot to the QML shell in both UEFI and BIOS QEMU tests.
- A VM screenshot must be captured and inspected.

The full twelve-app suite, lock screen, full Settings coverage, XWayland,
screencopy, workspace overview and production performance/accessibility sign-off
remain in the roadmap and are not implied by the Alpha 0.1 base.
