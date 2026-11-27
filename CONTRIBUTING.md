# Contributing to Materialized Linux

Thanks for wanting to help! Materialized Linux is an **alpha** project: the verified base is deliberately small, and the rest of the distribution is tracked openly in [docs/ROADMAP.md](docs/ROADMAP.md) and [docs/TODO.md](docs/TODO.md). Contributions that keep that honesty intact are very welcome.

## Ground rules

- **Real backends only.** Applications and Settings talk to real system services (D-Bus, `/proc`, PipeWire, the kernel backlight interface). No mock data (decision D-022).
- **No invented colors.** Every color role comes from the generated token/scheme pipeline. Components ask for roles by name (`M3Theme.secondaryContainer`); the token generator fails the build on undefined roles (D-011).
- **Keep the docs honest.** Do not represent unfinished roadmap items as done. When you complete something, update `docs/TODO.md` / `docs/ROADMAP.md` with evidence (the command that proves it and the artifact it produces).
- **Record architecture choices.** New architectural decisions go in `docs/DECISIONS.md` with rationale, following the existing D-xxx format.
- **Stack:** C++20, Qt 6 / QML, wlroots (vendored, pinned), CMake (D-032). The release target is Debian 13 (trixie) x86_64; Ubuntu 22.04 / WSL2 is the development compatibility environment.

## Development setup

WSL2 Ubuntu 22.04 or a Debian/Ubuntu host:

```bash
bash build/bootstrap-wsl.sh --all
bash build/bootstrap-runtime.sh
```

If WSL clock drift breaks APT (`Release file ... is not valid yet`) or Ninja keeps re-running CMake:

```bash
sudo bash build/fix-wsl-clock.sh
sudo bash build/normalize-source-times.sh
```

Optional: enable nested virtualization for KVM-accelerated VM tests (`%USERPROFILE%\.wslconfig`, `[wsl2]` `nestedVirtualization=true`, then `wsl --shutdown`). TCG works without it, just slower.

## Building

Unified build with presets (`dev`, `rel`, `nogui`, `asan`):

```bash
cmake --preset dev
cmake --build --preset dev -j4
ctest --test-dir build/dev --output-on-failure
```

Or build the session components individually - see [docs/BUILDING.md](docs/BUILDING.md) for the exact per-component commands and the ISO build (`sudo bash platform/iso/build.sh`).

## Testing your change

Run the checks for the area you touched:

| Change area | Checks |
|---|---|
| Color engine / tokens | `python3 -m unittest tests.unit.token_compiler_test -v` and `ctest --test-dir build/qml --output-on-failure` (m3theme) |
| Compositor | `bash build/test-compositor.sh` and `bash build/test-wayland-client.sh` |
| Applications | `bash build/build-core-apps.sh` (builds all nine apps + unit tests) and `bash build/test-new-apps.sh` (every app as a real Wayland client) |
| Theme / shell | `bash build/test-theme.sh` and `bash build/screenshot-shell.sh` |
| Installer core | `bash build/test-installer.sh` (dry-run only - never touches a real disk) |
| Session / ISO | `sudo bash platform/iso/build.sh`, then `bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode uefi` and `--mode bios` |

The VM harness gates on the `Materialized session started` serial marker from the loaded QML shell and on typed input reaching the terminal - a compositor-only process does not count as a working desktop. If your change affects the session, both firmware modes must pass.

## Pull request workflow

1. Branch from `main` (or fork) and keep the change focused.
2. Add or extend unit tests wherever there is logic to test.
3. Run the checks above for the area you touched; fix warnings you introduce.
4. For UI/session changes, include evidence: screenshots (`build/screenshots/`, `build/vm/`) or serial-log excerpts.
5. Update the relevant docs in the same PR (ROADMAP/TODO for status, DECISIONS for architecture, ARCHITECTURE for design, BUILDING for commands).
6. Open the PR describing what changed, how it was tested, and any roadmap impact.

Releases are cut with `build/release.sh` and tagged `vX.Y.Z`; release checksums and manifests are generated, not hand-edited.

## Where to start

The "Remaining work" list in [docs/TODO.md](docs/TODO.md) is the best place to pick something up:

- Music player and PackageKit/AppStream software center.
- Complete NetworkManager, BlueZ, audio and display settings pages.
- Lock screen/PAM and proper layer-shell panel placement.
- Graphical installer UI and unattended VM installation matrix.
- XWayland, gamma-control/Night Light and workspace overview.
- Accessibility, performance budgets and real GPU hardware validation.

## Reporting bugs

Open a [GitHub issue](https://github.com/Jihoo125/m3e-linux/issues) with:

- The ISO version (and SHA-256 if you built it yourself).
- For VM problems: hypervisor, firmware (UEFI/BIOS), graphics device, RAM/vCPUs, and whether Secure Boot is off - most boot problems are configuration, see [docs/RUNNING-ON-VMS.md](docs/RUNNING-ON-VMS.md).
- Reproduction steps and, if available, the serial log (`console=ttyS0`).

**Security vulnerabilities: do not open a public issue.** Follow [SECURITY.md](SECURITY.md) instead.

## License

By contributing, you agree that your contributions are licensed under the project's [MIT license](LICENSE.md). Vendored third-party sources under `third_party/` keep their own licenses.