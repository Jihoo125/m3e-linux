# Materialized Linux

<!-- Repository stats -->
![Stars](https://img.shields.io/github/stars/Jihoo125/m3e-linux?style=flat&logo=github)
![Forks](https://img.shields.io/github/forks/Jihoo125/m3e-linux?style=flat&logo=github)
![Watchers](https://img.shields.io/github/watchers/Jihoo125/m3e-linux?style=flat&logo=github)
![Issues](https://img.shields.io/github/issues/Jihoo125/m3e-linux)
![Pull Requests](https://img.shields.io/github/issues-pr/Jihoo125/m3e-linux)
![Last Commit](https://img.shields.io/github/last-commit/Jihoo125/m3e-linux)
![Commit Activity](https://img.shields.io/github/commit-activity/m/Jihoo125/m3e-linux)
![Repo Size](https://img.shields.io/github/repo-size/Jihoo125/m3e-linux)
![Code Size](https://img.shields.io/github/languages/code-size/Jihoo125/m3e-linux)
![Contributors](https://img.shields.io/github/contributors/Jihoo125/m3e-linux)

<!-- Project info -->
![License](https://img.shields.io/github/license/Jihoo125/m3e-linux)
![Version](https://img.shields.io/badge/version-0.2.1-blue)
![Status](https://img.shields.io/badge/status-alpha-orange)
![Platform](https://img.shields.io/badge/platform-x86__64-informational)
![Architecture](https://img.shields.io/badge/architecture-Intel%20%7C%20AMD-blue)
![OS](https://img.shields.io/badge/OS-Debian%2013-A81D33?logo=debian&logoColor=white)
![Kernel](https://img.shields.io/badge/kernel-Linux-FCC624?logo=linux&logoColor=black)

<!-- Technologies -->
![C++](https://img.shields.io/badge/C++-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-41CD52?logo=qt&logoColor=white)
![QML](https://img.shields.io/badge/QML-41CD52?logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-064F8C?logo=cmake&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)
![Bash](https://img.shields.io/badge/Bash-4EAA25?logo=gnubash&logoColor=white)
![Wayland](https://img.shields.io/badge/Wayland-FFBC00?logo=wayland&logoColor=black)
![wlroots](https://img.shields.io/badge/wlroots-compositor-informational)

<!-- Desktop / design -->
![Material Design](https://img.shields.io/badge/Material%20Design-757575?logo=materialdesign&logoColor=white)
![Material You](https://img.shields.io/badge/Material%20You-6750A4)
![Material 3](https://img.shields.io/badge/Material%203-6750A4)
![Dynamic Color](https://img.shields.io/badge/dynamic%20color-supported-6750A4)
![Dark Mode](https://img.shields.io/badge/dark%20mode-supported-222222)
![Light Mode](https://img.shields.io/badge/light%20mode-supported-FAFAFA?logoColor=black)

<!-- Build / testing -->
![Build System](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)
![Tests](https://img.shields.io/badge/tests-unit%20%2B%20QEMU-blue)
![Virtualization](https://img.shields.io/badge/VM-QEMU-FF6600?logo=qemu&logoColor=white)
![ISO](https://img.shields.io/badge/image-Live%20ISO-informational)
![UEFI](https://img.shields.io/badge/boot-UEFI-informational)
![BIOS](https://img.shields.io/badge/boot-BIOS-informational)

<!-- GitHub releases -->
![Release](https://img.shields.io/github/v/release/Jihoo125/m3e-linux)
![Latest Release](https://img.shields.io/github/release-date/Jihoo125/m3e-linux)
![Pre-release](https://img.shields.io/github/v/release/Jihoo125/m3e-linux?include_prereleases)
![Downloads](https://img.shields.io/github/downloads/Jihoo125/m3e-linux/total)

<!-- Community -->
![Open Source](https://img.shields.io/badge/open%20source-yes-brightgreen)
![Contributions](https://img.shields.io/badge/contributions-welcome-brightgreen)
![Issues Welcome](https://img.shields.io/badge/issues-welcome-blue)
![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen)
![Made with Linux](https://img.shields.io/badge/made%20with-Linux-FCC624?logo=linux&logoColor=black)
![Built Different](https://img.shields.io/badge/built-different-purple)
![Powered by C++](https://img.shields.io/badge/powered%20by-C++-00599C?logo=cplusplus&logoColor=white)
![No Telemetry](https://img.shields.io/badge/telemetry-none-success)

<!-- Badge style alternatives: flat | flat-square | plastic | for-the-badge | social -->

**A Material 3 desktop distribution for x86_64 Intel and AMD computers.**

> **Status (2026-11-21): Alpha 0.2.1 released.** Correctness release on top of
> 0.2.0: real keyboard/pointer input everywhere (the libinput backend was never
> created), keyboard focus for newly mapped windows, a visible software cursor
> on every virtual GPU, and removal of the 0.2.0 demo autostart — apps now open
> only via the launcher or Ctrl+Alt+T / Ctrl+Alt+B. The Debian 13 live ISO boots
> to the visible QML desktop in both UEFI and BIOS QEMU with end-to-end
> typed-input proof (`build/vm/input-uefi.png`, `build/vm/input-bios.png`), and
> boots on VMware Workstation Pro 25H2 (`build/vm/vmware-desktop.png`).
> ISO SHA-256: `c5d2b42c33d477f3a6480614822d8b98aadb05869f5f062afad469ebc1d73886`.
> Release checklist: [`docs/ALPHA-0.2.1.md`](docs/ALPHA-0.2.1.md).
> The complete distribution described in the roadmap is **not** finished: the
> music player, software center, full Settings coverage, lock screen, and the
> graphical installer wizard are still in progress.
>
> Do not install Alpha 0.1 on a production machine. Test it in a VM or on a
> dedicated spare machine.

## What is implemented

- **wlroots compositor (`matcomp`)** — real Wayland server with headless and
  hardware backends, xdg-shell windows, layer-shell surfaces, workspaces, focus,
  snapping, maximizing, fullscreen, XKB keyboard input, pointer input,
  wlr-screencopy screen capture, and app-launch hotkeys (Ctrl+Alt+T terminal,
  Ctrl+Alt+B browser).
- **Material 3 engine (`libm3`)** — CAM16/HCT color conversion, tonal palettes,
  dynamic schemes (including the Expressive variant), wallpaper quantization,
  seed selection, and contrast-aware color roles. No web UI or simulated values.
- **Material 3 Expressive** — spring-physics motion tokens (under-damped spatial
  springs, critically damped effects springs), press shape morphing, emphasized
  variable-weight typography, and the Expressive color variant selectable in
  Settings; covered by the `m3theme` unit test.
- **Live theme service (`matsessiond`)** — decodes a wallpaper, extracts a seed,
  generates light and dark schemes, writes artifacts atomically (including a
  Firefox `userChrome.css`), and exposes theme controls over D-Bus.
- **Material shell (`matshell`)** — top panel, application launcher and search,
  notification service, quick settings, real volume/brightness backends, XDG
  autostart, and a wallpaper-driven desktop.
- **Applications** — calculator, text editor, system monitor, file manager,
  Settings, terminal (real PTY + VT emulator), image viewer, screenshot tool
  (wlr-screencopy), and calendar (persistent local events), all with real
  backends and unit tests where logic exists.
- **Browser** — Firefox ESR packaged in the image and launched through the
  `matbrowser` wrapper with a Material 3 chrome generated from the live scheme;
  native Wayland, dedicated profile.
- **Installer core** — safe dry-run GPT planning, UEFI and BIOS layouts,
  ext4/Btrfs command generation, validation, subprocess handling, and cleanup.
- **Image pipeline** — Debian live rootfs, squashfs, GRUB hybrid UEFI/BIOS ISO,
  and a QEMU smoke-test harness.

## Current scope versus the full vision

| Area | Alpha 0.1 | Full roadmap |
|---|---|---|
| Wayland compositor | Working core, headless client test, screencopy, hotkeys | Lock, gamma/Night Light, XWayland, decorations |
| Dynamic theming | Wallpaper → scheme → live QML theme; Expressive variant + spring motion | Full contrast/density matrix and GTK compatibility polish |
| Desktop shell | Panel, launcher, notifications, quick settings, XDG autostart | Overview, workspaces UI, tray, OSD, gestures, lock screen |
| Applications | 9 apps + Firefox ESR: terminal, viewer, screenshot, calendar, calculator, editor, monitor, files, Settings | Software center, music player |
| Settings | Appearance, basic network info, hostname, timezone, keyboard, storage/about | Complete NetworkManager/BlueZ/UPower/UDisks2/PackageKit coverage |
| Installation | Safe installer core + live ISO build | Full graphical wizard, unattended install VM matrix, real-hardware validation |
| Platforms | x86_64, Debian 13, Wayland, systemd, GRUB hybrid UEFI/BIOS | UEFI/BIOS boot matrix and performance/accessibility sign-off |


The remaining work is tracked in [`docs/ROADMAP.md`](docs/ROADMAP.md) and
[`docs/TODO.md`](docs/TODO.md). [`docs/DECISIONS.md`](docs/DECISIONS.md) records
the architecture choices. To run the live ISO on QEMU/KVM, VirtualBox, VMware,
Hyper-V, GNOME Boxes/virt-manager or UTM, see
[`docs/RUNNING-ON-VMS.md`](docs/RUNNING-ON-VMS.md).

## Requirements

For development on this Windows machine, WSL2 Ubuntu is sufficient. For an ISO
build, use a Linux host or WSL2 with the following installed:

```bash
bash build/bootstrap-wsl.sh --all
bash build/bootstrap-runtime.sh
```

The scripts install GCC/CMake/Ninja, Qt 6, wlroots, Wayland development files,
D-Bus/system libraries, debootstrap, live-build, squashfs-tools, xorrisofs,
GRUB, and QEMU. Debian 13 is the release target; Ubuntu 22.04 is the development
compatibility environment and has older Qt/wlroots APIs.

If WSL clock drift causes `Release file ... is not valid yet` or Ninja repeatedly
re-runs CMake, run:

```bash
sudo bash build/fix-wsl-clock.sh
sudo bash build/normalize-source-times.sh
```

Nested virtualization is optional. If `/dev/kvm` is unavailable, QEMU uses TCG
and is slower. It can be enabled in `%USERPROFILE%\.wslconfig`:

```ini
[wsl2]
nestedVirtualization=true
```

Then run `wsl --shutdown` from Windows and reopen WSL.

## Build

Build the pieces used by the live session:

```bash
# In WSL, from the repository root
cmake -S libs/libm3 -B build/m3 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/m3 --parallel

cmake -S compositor/matcomp -B build/matcomp -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/matcomp --parallel

cmake -S libs/libm3-qt -B build/qml -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DQT_QML_OUTPUT_DIRECTORY="$PWD/build/qml"
cmake --build build/qml --parallel

cmake -S shell/matsessiond -B build/matsessiond -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/matsessiond --parallel

cmake -S shell/matshell -B build/matshell -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/matshell --parallel

bash build/build-core-apps.sh
```

The individual component CMake files intentionally support development on the
older WSL Qt 6.2 stack as well as the Debian 13 release stack. The release build
uses Qt 6.8 and the pinned wlroots compatibility layer.

## Run and test locally

### Compositor and a real Qt/Wayland client

```bash
bash build/test-compositor.sh
bash build/test-wayland-client.sh
```

### Live wallpaper-derived theme

```bash
bash build/test-theme.sh
```

This generates a scheme from `design/wallpaper/materialized-dawn.png`, loads it
in the QML gallery, and captures a screenshot in `build/screenshots/`.

### Shell screenshot

```bash
bash build/screenshot-shell.sh
```

### Unit tests

```bash
bash build/test-installer.sh
python3 -m unittest tests.unit.token_compiler_test -v
```

The installer tests cover JSON validation, UEFI/BIOS geometry, 4K alignment,
capacity and overflow, ext4/Btrfs command generation, password redaction,
subprocess I/O, and mount rollback. They do not touch real disks.

## Build the live ISO

The ISO builder creates a real Debian rootfs and installs the locally built
session into it. It does not use a website, screenshot, or desktop mockup.

```bash
sudo bash platform/iso/build.sh
```

The output is:

```text
build/iso/materialized-0.2.1-amd64.iso
build/iso/materialized-0.2.1-amd64.iso.sha256
```

The live environment uses a `materialized` account with password `materialized`.
That account is for evaluation only; change/remove it before using the image
outside a VM. TTY1 starts the Materialized session. Other TTYs remain available
for recovery.

## Test in QEMU

```bash
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode uefi
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode bios
```

The harness writes serial logs and screenshots to `build/vm/`. It checks that
Linux boots and that the live session reaches its session launcher. A boot log
alone is not considered proof that the desktop works.

## Repository map

```text
libs/libm3/          Material color engine
libs/libm3-qt/       Qt/QML theme singleton and Material controls
compositor/matcomp/  wlroots Wayland compositor
shell/matsessiond/   Wallpaper/theme/session service
shell/matshell/      Panel, launcher, notifications, quick settings
apps/                Core system applications
installer/           Safe installation engine
platform/            Packaging, live ISO and session integration
 design/             Material tokens and generated wallpaper
 tests/              Unit and VM tests
 docs/               Architecture, decisions, roadmap, build and task docs
```

## License and upstream credits

Materialized Linux uses Debian, Qt, wlroots, Linux, systemd, GRUB, PipeWire,
NetworkManager, BlueZ, UPower, UDisks2, PackageKit, Inter, and Noto. The Material
color implementation follows the public Material Color Utilities algorithms;
upstream reference sources are retained under `third_party/` with their license.

No telemetry is collected by the session components. Crash and test artifacts
stay local unless you explicitly share them.
