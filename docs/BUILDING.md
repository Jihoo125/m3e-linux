# Materialized Linux — Building (Alpha 0.1)

**Updated:** 2026-11-14. This file describes commands that exist in the tree,
not future placeholders.

## Environment

- Release target: Debian 13 (trixie), x86_64.
- Development environment: WSL2 Ubuntu 22.04 or a Debian/Ubuntu Linux host.
- Required: C++20 compiler, CMake 3.22+, Ninja, Qt 6, wlroots development
  headers, Wayland protocols, libxkbcommon, libinput, and the D-Bus libraries.
- ISO: root, debootstrap, live-build, squashfs-tools, xorrisofs, GRUB EFI and
  BIOS binaries, and Debian's current archive signing keys.
- VM: qemu-system-x86_64 and OVMF for UEFI. KVM is optional; TCG works but is
  slower.

Install development dependencies in WSL:

```bash
bash build/bootstrap-wsl.sh --all
bash build/bootstrap-runtime.sh
```

If WSL time drift breaks APT or incremental CMake:

```bash
sudo bash build/fix-wsl-clock.sh
sudo bash build/normalize-source-times.sh
```

## Unified build

```bash
cmake --preset dev
cmake --build --preset dev -j4
ctest --test-dir build/dev --output-on-failure
```

Presets:

- `dev`: Debug, shell, apps, compositor, installer, tests.
- `rel`: Release with link-time optimization.
- `nogui`: only the dependency-free color engine/tools.
- `asan`: AddressSanitizer/UBSan for the non-GUI subset.

The release target is Qt 6.8 / Debian 13. The development compatibility layer
also compiles against Qt 6.2 and wlroots 0.15 on Ubuntu 22.04.

## Build the session only

The ISO script expects the following artifacts. These are separate CMake
projects so development can proceed on an older host without forcing a newer Qt:

```bash
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

`build-core-apps.sh` builds all nine applications and runs the calculator,
text-editor, terminal-emulator and calendar unit tests.

## Verified local checks

```bash
bash build/test-compositor.sh
bash build/test-wayland-client.sh
bash build/test-new-apps.sh
bash build/test-theme.sh
bash build/screenshot-shell.sh
bash build/test-installer.sh
ctest --test-dir build/qml --output-on-failure   # M3 Expressive theme tokens
python3 -m unittest tests.unit.token_compiler_test -v
```

The compositor test launches a real Qt/Wayland client. `test-new-apps.sh` runs
every application (terminal, image viewer, calendar, screenshot, calculator,
editor, monitor, files, Settings) as a real Wayland client under the headless
compositor. The theme test decodes a real wallpaper, generates light/dark
schemes, and captures a QML gallery. The installer tests are dry-run and never
modify a disk.

## Build an ISO

First refresh Debian's archive trust chain on older Ubuntu hosts:

```bash
sudo bash build/refresh-debian-keyring.sh
sudo bash platform/iso/build.sh
```

The script:

1. Verifies the session binaries and stages them with the QML module,
   wallpaper, desktop entries and session launcher.
2. Creates a real Debian 13 live rootfs with systemd, NetworkManager, BlueZ,
   PipeWire, Mesa and the required Qt/wlroots libraries.
3. Installs the live user/session configuration and GRUB UEFI/BIOS boot entries.
4. Produces a squashfs root and a hybrid ISO at
   `build/iso/materialized-0.2.1-amd64.iso` with SHA-256 metadata.

The ISO work directory is `/var/tmp/materialized-iso` because Windows-mounted
`/mnt/c` cannot host Linux device nodes. Override with `MAT_ISO_WORK` on a Linux
filesystem, and `MAT_ISO_OUTPUT` to change the output filename.

## QEMU checks

For running the ISO on other hypervisors (VirtualBox, VMware, Hyper-V, GNOME
Boxes/virt-manager, UTM), including the graphics/firmware requirements and a
verified-vs-untested matrix, see [`RUNNING-ON-VMS.md`](RUNNING-ON-VMS.md).

```bash
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode uefi
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode bios
```

The harness waits for the `Materialized session started` marker from the loaded
QML shell. It does not count a compositor-only process as a working desktop.
It then captures a second screenshot after the session's XDG autostart has
launched the terminal and Firefox ESR (`build/vm/apps-<mode>.png`); an in-guest
watchdog reports the running application processes on the serial log.
Logs are saved in `build/vm/`. Use `--keep-running` to inspect the VM and
`--timeout 300` on a slow TCG host. The default timeout is 300 s; without
`/dev/kvm` access QEMU falls back to slow TCG emulation (run the harness with
sudo, or add your user to the `kvm` group, for hardware acceleration).

In the live session, Ctrl+Alt+T opens the terminal and Ctrl+Alt+B opens the
browser. The browser is Firefox ESR launched through `/usr/bin/matbrowser`,
which applies a Material 3 `userChrome.css` generated by `matsessiond` from the
live wallpaper scheme and uses a dedicated profile under
`~/.local/share/materialized/firefox-profile`.

## Live credentials

The evaluation image creates `materialized` with password `materialized`. This
is intentionally an evaluation credential, not a production account. Remove or
change it before using the image outside an isolated VM.

## Current limitations

The music player and software center, lock screen, full Settings coverage,
graphical installer UI, XWayland and release-quality hardware/accessibility
sign-off remain in the roadmap. Screencopy, the terminal, image viewer,
screenshot tool, calendar and the Firefox ESR browser are implemented and
verified. See `ALPHA-0.1.md` and `ROADMAP.md` for the exact completion state.
