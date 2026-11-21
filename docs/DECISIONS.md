# Materialized Linux — Decisions

**Status:** living document · **Last updated:** 2026-11-13

Every consequential choice is recorded here with its rationale and the
alternatives that were rejected. Numbers are permanent: a superseded decision is
marked **Superseded by D-0nn**, never deleted.

Format: **ID · Title · Status · Context · Decision · Alternatives rejected · Consequences**

---

## Stack & platform

### D-001 · Language and UI toolkit: C++20 + Qt 6 / QML · **Accepted**
**Context.** A custom Wayland compositor and shell need low-level control (compositor)
and rapid, animatable UI (shell + 12 apps). Material 3 demands continuous
geometry/colour animation.
**Decision.** C++20 for the compositor, session daemon, engine, and app controllers;
Qt 6.8 + QML for all UI. One toolchain, one language, shared `libm3`.
**Rejected.**
- *Rust + Smithay + iced/egui*: excellent compositor ergonomics, but no mature
  retained-mode widget toolkit with M3-grade animation; we'd write a widget library
  from scratch for 12 apps.
- *wlroots in C + GTK4 shell*: GTK4 CSS cannot express M3 state layers, emphasized
  easing, or dynamic role rebinding at runtime without fighting the toolkit.
- *GNOME Shell fork*: fastest path, but "custom desktop shell" would be a theme, and
  JS/GJS performance and extension fragility conflict with G3.
- *Flutter/Rust+Slint/Compose Multiplatform*: non-native Wayland clients, poor
  a11y (AT-SPI), heavier runtime.
**Consequences.** We own a QML component library (`libm3-qt`) — a real cost, paid
once in P1 and reused by every app. Qt baseline is 6.8 (Debian trixie); newer
features are guarded by `MAT_QT_NEXT`.

### D-002 · Compositor core: wlroots (vendored, pinned) · **Accepted**
**Context.** Writing a Wayland protocol + DRM/KMS implementation from scratch is
years of work and a security minefield.
**Decision.** Build on wlroots, pinned to an exact tag in
`third_party/manifest.json`, with all direct wlroots calls funnelled through
`compositor/matcomp/wlr_compat.hh`.
**Rejected.** *KWin/Wayland* (Qt-based but KDE-frameworks-heavy, hard to restyle to
M3, C++/Qt version lockstep); *write our own* (G2/G3 achievable without it);
*Weston* (reference compositor, poor tiling/extension story); *Smithay* (Rust,
see D-001).
**Consequences.** API churn risk (0.18→0.19 renamed renderer/scene APIs) is
contained by the compat shim and the pin. wlroots gives us layer-shell,
screencopy, gamma-control, idle-notify, session-lock, and XWayland for free.

### D-003 · Base distribution: Debian 13 "trixie" · **Accepted**
**Context.** Need APT, systemd, modern Wayland stack, and a stable base.
**Decision.** Debian 13. GCC 14, glibc 2.41, kernel 6.12, Qt 6.8, GTK 4.16, Mesa 25.
**Rejected.** *Ubuntu 24.04 LTS* (Qt 6.4, GTK 4.14 — older M3-relevant toolkits,
plus snap dependency we don't want); *Debian sid* (not reproducible);
*Arch* (no APT, rolling breakage conflicts with G5).
**Consequences.** We ship our own Qt-6.8-targeted components; ISO builds pin a
`snapshot.debian.org` timestamp for reproducibility. Ubuntu users can still
develop (see BUILDING.md §0 for the package differences).

### D-004 · Display server: Wayland-only compositor, X11 via XWayland · **Accepted**
**Context.** X11 is unmaintainable for a new compositor in 2026.
**Decision.** Wayland is the only native path. XWayland is a build flag
(`-DMAT_ENABLE_XWAYLAND=ON`, default on) for legacy apps.
**Rejected.** *X11 default with Wayland session* (inverts the requirement, doubles
the surface we must maintain).
**Consequences.** Screenshots/screen-record must use `wlr-screencopy` /
`ext-image-copy-capture`, and the portal mediates for sandboxed apps. X11 apps get
theme colours via `xrdb` (static path, §5 of ARCHITECTURE.md).

### D-005 · Boot: systemd, no display manager · **Accepted**
**Context.** Requirement says systemd. A greeter (GDM/SDDM) adds boot time and a
second theming surface.
**Decision.** systemd units; `matcomp` runs as a systemd service on `seat0` and
starts the session stack itself. Multi-user machines can enable
`matcomp --greeter` mode later (P7).
**Rejected.** *SDDM/GDM* (boot budget G3, theming inconsistency); *no systemd*
(requirement).
**Consequences.** Session lock and user switching are our responsibility
(`ext-session-lock-v1` + PAM in `matlock`). getty on tty1–6 always remains as a
recovery path.

### D-006 · Filesystems: ext4 default, Btrfs optional with subvolumes · **Accepted**
**Context.** Requirement. Btrfs enables snapshots but complicates recovery.
**Decision.** Installer offers ext4 (default) or Btrfs. Btrfs layout:
`@` (root), `@home`, `@var/log`, `@snapshots`, each a subvolume; `.snapshots`
mounted read-only. Swap is a **swapfile** on ext4 or **zram** on Btrfs (Btrfs
swapfiles have constraints).
**Rejected.** *Btrfs default* (recovery complexity for new users, GRUB
`/boot` on Btrfs needs care); *ZFS* (licensing/DKMS burden).
**Consequences.** `matinstall-core` needs two format paths and fstab generators;
Btrfs snapshot tooling is exposed in Settings → Storage (P5).

### D-007 · Bootloader: GRUB 2, hybrid UEFI + BIOS · **Accepted**
**Context.** Requirement + must boot from a hybrid ISO.
**Decision.** GRUB 2. UEFI: `grubx64.efi` on the ESP. BIOS: `i386-pc` core into the
MBR gap with a `bios_grub` partition on GPT. ISO carries both an El Torito EFI image
and an `i386-pc` boot image.
**Rejected.** *systemd-boot* (UEFI-only, fails the legacy BIOS requirement);
*rEFInd* (not in Debian main by default).
**Consequences.** Must ship `grub-efi-amd64-bin` + `grub-pc-bin` + `mtools`;
`os-prober` enabled for dual-boot detection.

---

## Design system

### D-010 · Colour engine: from-spec CAM16/HCT in `libm3` (C++), verified against material-color-utilities · **Accepted**
**Context.** Material You requires HCT (Hue from LCh, Chroma from LCh, Tone = L*).
HSV/HSL approximations produce visibly wrong tonal palettes, especially for
saturated and near-neutral hues — the exact cases wallpapers produce.
**Decision.** Implement CAM16 forward/inverse, HCT solve (bisection + Newton),
tonal palettes, dynamic schemes, Celebi quantization, and Score exactly as
specified in material-color-utilities. Unit tests pin ARGB outputs for known seeds;
a Python cross-check (`tools/m3-reference/`) validates the C++ engine against the
upstream reference implementation where it can be installed.
**Rejected.** *Port of a Qt KColorScheme-style HSV engine* (fails G2 numerically);
*shell out to Python MCU at runtime* (adds a Python runtime dep to the session,
slow, fragile); *precomputed palette LUT only* (cannot do arbitrary wallpaper seeds).
**Consequences.** `libm3` is dependency-free C++20 and testable on any host —
including Windows without Qt. This was the first thing built and the thing most
likely to be wrong, so it carries the heaviest test burden.

### D-011 · Tokens as the single source of truth; generated artifacts · **Accepted**
**Context.** G1 requires one coherent design language across QML, GTK, X11, icons.
**Decision.** `design/tokens/*.json` → `tools/m3-token-compiler` → `generated/`
(`theme.qml`, `gtk.css`, `xrdb`, `scheme.json`, `colors.css`, `qt-palette.conf`).
No literal colours in `apps/` or `shell/`; enforced by a lint test.
**Rejected.** *Hardcoding colours per component* (drifts immediately);
*runtime-only theming with no tokens* (GTK/X11 cannot subscribe).
**Consequences.** Build-time codegen step; `generated/` is never committed. Adding
a role means editing tokens and every consumer updates automatically.

### D-012 · Dynamic colour propagation: live JSON + generated static CSS/xrdb · **Accepted**
**Context.** QML can subscribe to changes; GTK4/X11 cannot.
**Decision.** Two paths (ARCHITECTURE.md §5): live reload of `scheme-<mode>.json`
for `libm3-qt`, and generated `gtk.css`/`qt-palette.conf`/`xrdb` for everything else,
re-exported into the systemd user environment.
**Rejected.** *Force every app to be QML* (unrealistic — GTK apps will exist);
*D-Bus-only with no files* (no crash-safe fallback, harder to debug).
**Consequences.** GTK apps recolour on restart, not live. Documented as a known
limitation; libadwaita override mitigates the worst cases.

### D-013 · Rounded corners & shadows drawn server-side by the compositor · **Accepted**
**Context.** G1 demands consistent window decoration across *all* apps including
X11 and non-M3 ones.
**Decision.** `matcomp` draws radius (12 dp) + elevation shadow as scene-graph
shader nodes for surfaces that accept server-side decoration.
**Rejected.** *Client-side only* (inconsistent — GTK CSD, Qt, X11 all differ);
*per-client theme negotiation* (cannot enforce).
**Consequences.** Compositor owns a small shader set; clients using CSD
(GTK/libadwaita) are excluded and get their own radii, documented in the theming guide.

### D-014 · Typography: Google Sans unavailable → Inter as the M3 typeface · **Accepted**
**Context.** Material 3 specifies Roboto/Google Sans. Google Sans is not freely
redistributable.
**Decision.** Inter (SIL OFL) as the primary Latin/UI face with Roboto as fallback
where installed; Noto Sans/Serif + Noto CJK + Noto Color Emoji for coverage.
Type-scale tokens map M3 names (displayLarge…labelSmall) to Inter weights/sizes
with matching line-heights and letter-spacing.
**Rejected.** *Roboto only* (poorer on-screen hinting at small sizes on Linux, and
visually identical-to-Android rather than our own identity); *bundling Google Sans*
(licensing).
**Consequences.** Font packages are in the ISO by default
(`fonts-inter fonts-roboto-unhinted fonts-noto-*`). Rendering tuned via
fontconfig snippet in `platform/configs/`.

### D-015 · Icons: custom Material-symbol-consistent set + symbolic fallback · **Accepted**
**Context.** G1 consistency; we cannot ship Google's Material Symbols (licence +
identity).
**Decision.** Draw a Materialized icon set on a 24 dp grid, 2 dp keyline,
consistent stroke/terminal geometry; export SVG → PNG (16/22/24/32/48/64) +
symbolic variants. Fall back to `hicolor`/Adwaita for apps we don't cover.
**Rejected.** *Reuse Papirus/Adwaita wholesale* (inconsistent geometry, no M3 feel);
*Material Symbols directly* (licensing/identity).
**Consequences.** Real illustration work; `tools/icon-build.py` automates export and
generates tinted variants from the scheme for monochrome roles.

### D-016 · Motion: tokenized M3 easing + duration, compositor-side only for geometry · **Accepted**
**Context.** G3 performance + M3 motion feel.
**Decision.** `design/tokens/motion.json` defines the M3 curves
(`emphasized`, `emphasizedDecelerate/Accelerate`, `standard`, `legacy`) as cubic
bezier control points and duration tokens. QML components animate their own
internals; `matcomp` animates only window geometry (snap/overview/minimize) with a
small spring/easing engine using the same tokens.
**Rejected.** *Per-component ad-hoc easing* (inconsistent); *animating surfaces in
clients for window moves* (tearing, latency).
**Consequences.** One motion vocabulary everywhere; compositor animation code is
small and shared with the shell via the token file.

---

## Components

### D-020 · Web browser: package Firefox ESR + generated M3 chrome, don't fork · **Accepted**
**Context.** Requirement lists a web browser. Writing a browser engine is out of
scope (non-goal).
**Decision.** Ship Firefox ESR, branded, with `userChrome.css`/`userContent.css`
**generated from tokens**, plus Materialized prefs (search engine, homepage,
Do-Not-Track, no telemetry, Wayland-native by default). Optionally offer Chromium
in `--variant full`.
**Rejected.** *Fork a browser engine* (years); *ship unthemed Firefox* (breaks G1);
*WebKitGTK-based custom browser* (a real option later, but not for v1).
**Consequences.** Browser chrome recolours on restart, not live. `matweb` is a
packaging + theming component, not an engine.

### D-021 · Terminal: GTK4 + VTE, themed via generated CSS · **Accepted**
**Context.** Requirement lists a terminal. VTE is the mature, correct PTY/rendering
stack; writing a terminal emulator is a non-goal.
**Decision.** `matterminal` is a GTK4 + libvte app styled by generated M3 GTK CSS,
with tabs/splits, profiles, copy-mode, and M3 keybindings.
**Rejected.** *QML terminal with custom glyph renderer* (huge effort, poor
performance vs VTE's SIMD glyph cache); *ship GNOME Terminal unthemed* (breaks G1).
**Consequences.** One GTK app in an otherwise-QML tree; it depends on the static
CSS path (D-012). Documented exception to D-001.

### D-022 · Settings/app backend: real D-Bus system services, no mock data · **Accepted**
**Context.** G4. A settings app that fakes toggles is worthless.
**Decision.** Each Settings page has a controller binding to the real service:
NetworkManager (Wi-Fi/VPN), BlueZ (Bluetooth), UPower (battery), UDisks2 (storage),
logind (power/session), timedated (clock/tz), localed (locale/keyboard),
hostnamed (hostname), PackageKit (updates), matsessiond (appearance), power-profiles-daemon.
**Rejected.** *Mock/simulated settings* (explicitly forbidden); *shelling out to
`nmcli`/`bluetoothctl` and parsing text* (fragile, no async signals, breaks on
locale).
**Consequences.** `libmatdbus` must wrap a fair number of interfaces; those daemons
become runtime dependencies of `mat-desktop`.

### D-023 · Software Center: PackageKit over APT (+ optional Flatpak) · **Accepted**
**Context.** Requirement: APT package management. A software center needs async
transactions, progress, and metadata.
**Decision.** `matstore` uses PackageKit-Qt6 driving the APT backend, with AppStream
for screenshots/descriptions. Flatpak support is optional and off by default.
**Rejected.** *Direct `apt` subprocess management* (no transaction abstraction,
locking pain); *Snap* (not Debian, conflicts with D-003); *Flatpak-only*
(excludes the APT requirement).
**Consequences.** `packagekit` + `appstream` are runtime deps; PackageKit's APT
backend is slower than raw apt for large transactions — mitigated with progress UI
and background refresh throttling.

### D-024 · Notifications: `matshell` implements `org.freedesktop.Notifications`; rules/DND owned by `matsessiond` · **Accepted**
**Context.** The shell is the only always-on UI; but DND/rules must survive shell
restarts.
**Decision.** Shell = transport + presentation + history DB. Session daemon =
policy (DND, per-app rules, persistence). Shell queries the daemon on start.
**Rejected.** *Policy in the shell* (lost on shell restart/crash);
*separate notification daemon* (extra process, extra IPC hop, G3).
**Consequences.** Clean split; shell can be restarted (`matshell --restart`) without
losing user settings.

### D-025 · Lock screen: `ext-session-lock-v1` + PAM in `matlock` · **Accepted**
**Context.** Must lock before the compositor releases input; must authenticate as
the user without root helpers.
**Decision.** Compositor grabs input and hands outputs to `matlock`; `matlock`
authenticates via PAM (`materialized-lock` service: `pam_unix`, optional
`pam_fprintd`). Lock surface is created **before** the grab is acknowledged.
**Rejected.** *Reusing swaylock/gtklock* (won't match M3, extra deps);
*compositor-internal password entry* (compositor would need PAM + secrets — larger
attack surface).
**Consequences.** We must handle PAM conversation async in QML and never leak the
lock; screenshots are disabled while locked.

### D-026 · Idle/power: `matsessiond` state machine over `ext-idle-notify-v1` + logind inhibitors · **Accepted**
**Context.** Dim → lock → DPMS off → suspend must be coordinated and inhibitible
(video playback, presentations).
**Decision.** Session daemon owns the state machine; compositor reports idle;
logind inhibitors (from players, `matlock`) pause it.
**Rejected.** *Per-app timers* (inconsistent, G1); *compositor-owned policy*
(shouldn't survive compositor restart semantics, and needs user settings).
**Consequences.** One place to configure idle timings; Settings → Power binds here.

### D-027 · Privilege: only the installer core is privileged, gated by polkit · **Accepted**
**Context.** G4 + security. Minimise root surface.
**Decision.** `matinstall-core` runs as a system-bus service gated by polkit rule
`org.materialized.install`. Compositor, shell, and apps run as the user.
**Rejected.** *setuid helpers* (larger attack surface); *running the compositor as
root* (unnecessary; logind grants DRM master).
**Consequences.** Installer needs a polkit policy file + rule; live session grants
the live user install rights by group membership.

### D-028 · No telemetry; opt-in local crash reports · **Accepted**
**Context.** Trust and Debian norms.
**Decision.** No network telemetry anywhere. Crashes go to
`/var/lib/materialized/crash` only if the user opts in; `matreport` bundles them for
manual sharing.
**Rejected.** *Any always-on reporting.*
**Consequences.** We lose aggregate failure data; compensated by the VM test matrix
(P7) and opt-in reporting.

---

## Build & release

### D-030 · Reproducible builds: pinned snapshot + SOURCE_DATE_EPOCH + pinned third-party · **Accepted**
**Context.** G5.
**Decision.** `debootstrap` against a `snapshot.debian.org` timestamp recorded in
`platform/iso/snapshot.txt`; `SOURCE_DATE_EPOCH` honoured; `third_party/manifest.json`
pins upstream tarballs by SHA-256; `dpkg-buildpackage -j1`.
**Rejected.** *Floating mirrors* (non-reproducible); *git submodules without hashes*
(weaker integrity guarantee).
**Consequences.** `platform/iso/verify-reproducible.sh` can prove bit-identical
rebuilds. Snapshot bumps are deliberate, reviewed changes.

### D-031 · Hybrid ISO with overlay-root initramfs · **Accepted**
**Context.** Must boot UEFI and BIOS, run live, and support optional persistence.
**Decision.** Hybrid ISO (El Torito EFI + `i386-pc`). Live root = read-only
squashfs + tmpfs overlay via our own initramfs hook (not casper). Optional
persistence to a labelled partition.
**Rejected.** *casper* (Ubuntu-specific, D-003); *systemd-homed only* (not
appropriate for the live session).
**Consequences.** We own the live initramfs hook; persistence is a documented
opt-in (`persistence` label).

### D-032 · CMake (not Meson) for our tree; Meson allowed for vendored wlroots · **Accepted**
**Context.** Qt 6's tooling (`qt_add_qml_module`, `qmlcachegen`, `qt6_add_translations`)
is CMake-first; wlroots upstream uses Meson.
**Decision.** Our components use CMake with `CMakePresets.json`. Vendored wlroots
builds with Meson under `third_party/` and is consumed via a CMake imported target.
**Rejected.** *All-Meson* (Qt QML module support is weaker); *all-CMake* (would
mean patching wlroots' build).
**Consequences.** Two build systems at the boundary, isolated in `third_party/`.

### D-033 · Testing strategy: pure-logic unit tests host-agnostic; compositor/VM tests Linux-only · **Accepted**
**Context.** Development happens on Windows/WSL without KVM; releases are Linux.
**Decision.** `libm3`, the token compiler, parsers, and the partition planner are
unit-tested with Catch2/QtTest and run anywhere. Compositor uses wlroots'
**headless** backend in CI. Full boot/install/screenshot/perf tests run in QEMU on
Linux/KVM hosts (WSL2 with nested virt, or real hardware).
**Rejected.** *Requiring KVM for all tests* (blocks Windows contributors);
*skipping VM tests* (violates "test in a virtual machine" requirement).
**Consequences.** Two CI tiers. `tests/vm` degrades to TCG when `/dev/kvm` is
absent (slow but still runs).

---

### D-034 · Alpha 0.1 scope: prove a runnable base before broadening the suite · **Accepted**
**Context.** The full distribution is a large project (compositor, shell, twelve
applications, installer and release validation). Shipping placeholders would hide
whether the central desktop actually works.
**Decision.** Alpha 0.1 is limited to a real compositor, dynamic Material theming,
a usable shell, five core applications, the safe installer engine, and a live ISO
with QEMU boot acceptance. Remaining applications and system integration stay on
the public roadmap.
**Consequences.** README and roadmap distinguish verified behavior from future
features. A generated ISO is not enough: the QML shell must load in both firmware
modes before the alpha is called complete.

### D-035 · Older-host live-build compatibility stays project-local · **Accepted**
**Context.** Development uses Ubuntu 22.04 while the target is Debian 13. The host
live-build has older keyrings and mirror assumptions.
**Decision.** Verify Debian's published signing keys, install a current keyring,
use a Linux-native `/var/tmp` rootfs workspace, and keep mirror-layout adjustments
inside the project build configuration rather than editing system build tools.
**Consequences.** Signature verification stays enabled. The ISO builder works on
older WSL hosts while targeting the current Debian release.


| ID | Question | Leaning | Decide by |
|---|---|---|---|
| D-100 | Snapshots: Btrfs + `snapper` vs our own minimal snapshot tool | own minimal tool, snapper optional | P5 |
| D-101 | Immutable/atomic updates (OSTree-style) for v2 | out of scope for v1 | post-v1 |
| D-102 | On-screen keyboard: `matkeyboard` (Qt VK) vs reuse `squeekboard`/`maliit` | own Qt VK for M3 consistency | P4 |
| D-103 | Screen reader: Orca via AT-SPI vs custom | Orca (mature, AT-SPI) | P7 |
| D-104 | Container/Flatpak sandboxing by default for third-party apps | opt-in for v1 | post-v1 |
| D-105 | Multi-seat / multi-monitor mixed-DPI scaling | fractional scaling via wp-fractional-scale | P2 |
