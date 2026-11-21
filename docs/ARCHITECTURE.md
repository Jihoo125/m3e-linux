# Materialized Linux — Architecture

**Status:** living document · **Last updated:** 2026-11-13
**Audience:** contributors who need to understand *why* the pieces fit together the way they do.

Materialized Linux is a Debian 13 (trixie) based x86_64 distribution with a custom
Wayland compositor, a custom Material 3 desktop shell, and a suite of Material 3
system applications. This document describes the system from firmware to pixels.

---

## 1. Design goals and constraints

| # | Goal | Architectural consequence |
|---|------|---------------------------|
| G1 | One coherent design language across the whole OS | A single **token source of truth** (`design/tokens/*.json`) compiled into every consumer (QML, C++, GTK CSS, xrdb, icons). No hand-written colors anywhere in the tree. |
| G2 | Real dynamic color (Material You) | A from-spec **CAM16/HCT** implementation (`libm3`), not an HSV approximation. Tone → sRGB round-trip must match the reference algorithm bit-for-bit within 1/255. |
| G3 | Smooth 60–144 fps animation on a 4 GB machine | Compositor does **damage-tracked** scene-graph compositing only. Shell animations run in Qt Quick's scene graph on the client side; the compositor animates only window geometry (snap, workspace switch, minimize). |
| G4 | Everything is a real Linux system component | No mock UI. Settings talks to NetworkManager/BlueZ/UPower/UDisks2/logind/timedated over D-Bus. The Software Center drives APT via PackageKit. |
| G5 | Reproducible, scriptable build | Pinned upstream sources with SHA-256 (`third_party/manifest.json`), Debian snapshot mirror pinning, container-friendly build entry points. |
| G6 | Installable on UEFI and legacy BIOS | GRUB 2 with both `i386-pc` (BIOS, `bios_grub` partition) and `x86_64-efi` (ESP) paths; hybrid ISO image. |

**Non-goals:** writing a web browser engine, a terminal emulator from scratch where VTE
already exists, or an Android-app compatibility layer. See `DECISIONS.md` D-020/D-022.

---

## 2. Layer diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  USER-SPACE SESSION (per-user, systemd --user)                              │
│                                                                             │
│  Qt Quick / QML apps (matfiles, matsettings, matterminal*, matstore, …)     │
│      └── libm3-qt  (M3Palette, M3Typography, M3Motion, attached props)      │
│  GTK4 apps (*matterminal, third-party)                                      │
│      └── generated mat-theme-gtk.css                                        │
│                                                                             │
│  matshell ──── panel · launcher · quick settings · notifications · OSD      │
│     │         workspace overview · lock (ext-session-lock)                  │
│     │  wayland clients on wlr-layer-shell + xdg-shell                       │
│     ▼                                                                       │
│  matsessiond ─── theme engine (libm3) · wallpaper · power · idle · night    │
│     │           light · D-Bus: org.materialized.*                           │
│     ▼                                                                       │
├─────┼───────────────────────────────────────────────────────────────────────┤
│  matcomp  (C++20 Wayland compositor on wlroots)                             │
│     │   scene graph · DRM/KMS · input (libinput) · seat · XWayland          │
│     │   protocols: xdg-shell, layer-shell, ext-session-lock, ext-workspace, │
│     │   gamma-control, output-management, screencopy, pointer-constraints   │
├─────┼───────────────────────────────────────────────────────────────────────┤
│  KERNEL (Linux 6.12, Debian trixie)                                         │
│     DRM/KMS · evdev · io_uring · cgroups v2 · namespaces · Btrfs/ext4        │
└─────────────────────────────────────────────────────────────────────────────┘
   SYSTEM SERVICES: systemd · logind · NetworkManager · BlueZ · UPower ·
                    UDisks2 · PackageKit · rtkit/pipewire · polkit
```

`*` = documented exception to the "everything is QML" rule (see DECISIONS.md).

---

## 3. Repository layout

```
materialized-linux/
├── docs/                  ARCHITECTURE.md ROADMAP.md BUILDING.md TODO.md DECISIONS.md
├── design/
│   ├── tokens/            colour-roles.json  typography.json  shape.json  motion.json
│   │                      elevation.json  density.json  iconography.json
│   ├── specs/             per-component visual/interaction specs (Markdown)
│   └── wallpaper/         seeded default wallpapers (SVG sources + PNG exports)
├── libs/
│   ├── libm3/             C++20 Material 3 engine — CAM16, HCT, tone palettes,
│   │                        dynamic schemes, colour quantizers, contrast, tokens
│   ├── libm3-qt/          Qt6/QML plugin: M3Theme singleton, M3Palette value type,
│   │                        M3Button/M3Switch/M3Slider/M3Dialog/M3Menu components
│   ├── libmatdbus/        thin D-Bus binding layer (Qt6 D-Bus, no KDE deps)
│   └── mattoolkit/        C++ utilities shared by compositor/sessiond/installer
├── compositor/
│   ├── matcomp/           the compositor
│   └── protocols/         vendored/copied wayland XML + generated bindings
├── shell/
│   ├── matsessiond/       session daemon (theming, wallpaper, power, idle)
│   ├── matshell/          QML shell: panel, launcher, quicksettings, notifications
│   ├── matlock/           lock screen (ext-session-lock client)
│   └── matwallpaper/      background layer client (blurred/duotone wallpaper)
├── apps/
│   ├── matfiles/ matsettings/ matterminal/ mattext/ matsysmon/ matstore/
│   ├── matphoto/ matscreenshot/ matcalc/ matcalendar/ matmusic/
│   └── matweb/            Firefox ESR packaging + generated M3 chrome theme
├── installer/
│   ├── matinstall/        QML frontend
│   └── matinstall-core/   C++ deployment engine (partition → format → copy → configure)
├── platform/
│   ├── systemd/           units, presets, sysusers, tmpfiles, journald/logind config
│   ├── packages/          debian/ control files + meta packages (mat-desktop, mat-core…)
│   ├── configs/           /etc defaults: polkit rules, dbus, udev, sysctl, fonts
│   ├── portal/            xdg-desktop-portal-materialized (screenshot, filechooser, settings)
│   └── iso/               live ISO build (debootstrap → squashfs → xorriso hybrid)
├── third_party/           manifest.json (pinned upstream tarballs + sha256) + vendored code
├── build/                 bootstrap scripts, CMake presets, CI definitions
└── tests/
    ├── unit/              Catch2/QtTest suites (libm3 verified against MCU fixtures)
    ├── vm/                QEMU harness (UEFI + BIOS), boot-to-desktop checks
    └── autopilot/         scripted UI tests driven over evdev/wlroots test backend
```

---

## 4. Component inventory

### 4.1 `libm3` — Material 3 engine (C++20, no dependencies)

The heart of the design system. Pure computation, fully unit-testable on any host.

| Module | Responsibility |
|---|---|
| `math/` | sRGB ↔ linear ↔ XYZ(D65) ↔ L*a*b* ↔ LCh ↔ CAM16 (JCh/UCS) ↔ HCT; `Sanitize`/`ToInt`/`Sign` helpers; the CAM16 forward/inverse transforms with the M3 viewing conditions (`200/π` adapting luminance, `La = 11.725…`, surround average). |
| `hct/` | `Hct` (hue/chroma/tone → ARGB), `HctSolver` (bisection + Newton on the CAM16 inverse), `ViewingConditions`. |
| `palettes/` | `TonalPalette` (tones 0–100), `CorePalette` (primary/secondary/tertiary/neutral/neutralVariant/error). |
| `scheme/` | `DynamicScheme` + variants: `SchemeContent`, `SchemeExpressive`, `SchemeFidelity`, `SchemeMonochrome`, `SchemeNeutral`, `SchemeTonalSpot`, `SchemeVibrant`; `Variant`, `Contrast` levels. |
| `quantize/` | `QuantizerCelebi` (Wu box quantization → k-means in HCT, M3's wallpaper picker), `QuantizerWu`, `QuantizerMap`, `QuantizerWsmeans`. |
| `score/` | `Score::score(cam16 hues/chromas/proportions) → seed colours` (the M3 "best colour from image" ranking: dedupe near-hues, prefer chroma, penalise blue-ish neutrals). |
| `temperature/` | `TemperatureCache` (relative warm/cool ordering by CAM16 hue, used by expressive schemes). |
| `tokens/` | Loads `design/tokens/*.json`, resolves **role → HCT → ARGB** tables, and emits artifacts: `scheme.json`, `theme.qml`, `gtk.css`, `qt.palette`, `xrdb`, `colors.css`. |

**Public contract (stable):**

```cpp
namespace m3 {
  struct Hct { double hue, chroma, tone; static Hct From(uint32_t argb); uint32_t ToArgb() const; };
  class TonalPalette { public: static TonalPalette FromInt(uint32_t argb);
                                  static TonalPalette FromHct(Hct); uint32_t Tone(int) const; };
  struct DynamicScheme { Variant variant; Contrast contrast; bool isDark;
                         uint32_t sourceColorHct; /* … palettes … */
                         uint32_t GetPrimary() const; uint32_t GetSurface() const; /* ~60 roles */ };
  std::vector<uint32_t> QuantizeCelebi(const std::vector<uint32_t>& pixels, int count);
  std::vector<uint32_t> Score(const std::map<uint32_t,double>& camHistogram);
}
```

Determinism rule: all tone→ARGB output is rounded with `std::round` on the *linearised*
channel then clamped, matching material-color-utilities. Fixtures in `tests/unit/m3`
pin the exact ARGB values for a set of known seeds so any drift fails CI.

### 4.2 `libm3-qt` — QML design system

A Qt Quick plugin exposing:

* `M3Theme` singleton — live scheme object; `M3Theme.primary`, `.onPrimary`,
  `.surfaceContainerHigh`, `.isDark`, `.radiusL`, `.emphasizedEasing` …; emits
  `schemeChanged` when `matsessiond` publishes a new scheme.
* Value types: `M3Color` (role-keyed colour), `M3TypeScale` (labelMedium, titleLarge…).
* Components: `M3Button`, `M3FilledTonalButton`, `M3OutlinedButton`, `M3TextButton`,
  `M3FAB`, `M3IconButton`, `M3Switch`, `M3Checkbox`, `M3Radio`, `M3Slider`,
  `M3TextField`, `M3SegmentedButton`, `M3Card`, `M3Dialog`, `M3Menu`, `M3Snackbar`,
  `M3NavigationRail`, `M3TopAppBar`, `M3Chip`, `M3Progress`, `M3ListItem`.
* Motion: `M3Motion` with the standard easing curves baked as `easing.bezierCurve`
  (`emphasized`, `emphasizedDecelerate`, `emphasizedAccelerate`, `standard`, `legacy`).

Every component reads exclusively from `M3Theme` — no literal hex codes are permitted
in `apps/` or `shell/` (enforced by `tests/unit/no-literal-colours.py` scanning the tree).

### 4.3 `matcomp` — the compositor

C++20 over **wlroots** (vendored, pinned). Process model: single process, one
event loop (`wl_event_loop` + `libuv`-free, we use wlroots' own loop), render thread
= main thread (Wayland compositors are latency-critical; no multi-threaded GL).

Subsystems:

| File group | Responsibility |
|---|---|
| `backend/` | wlroots backend selection (DRM/libinput/headless), output hotplug, mode setting, adaptive sync. |
| `render/` | scene-graph output, damage tracking, `wlr_renderer` (GLES2, Vulkan when available), gamma LUT upload for Night Light. |
| `input/` | libinput devices, seat/keyboard group, keymap (XKB), pointer constraints, tablet, gestures (3/4-finger swipe → workspace), touch. |
| `window/` | `xdg_toplevel` lifecycle, decorations (server-side M3 CSD: rounded corners drawn into a subsurface), focus/stacking, minimize, maximize/fullscreen. |
| `tiling/` | snap zones (edges + corners), `Super+drag` resize, tiling layouts (columns/master-stack), workspace trees. |
| `workspaces/` | `ext-workspace-unstable-v1` implementation; per-output workspace groups. |
| `lock/` | `ext-session-lock-v1` — grabs input, locks outputs, hosts `matlock`. |
| `screencopy/` | `wlr-screencopy-v1` + `ext-image-copy-capture-v1` for `matscreenshot` and the portal. |
| `xwayland/` | optional X11 compatibility; `XDG_CURRENT_DESKTOP=Materialized`, xrdb colours from the theme. |
| `anim/` | small spring/easing engine for geometry animations using M3 motion tokens. |
| `ipc/` | `matcomp` control socket (Unix, JSON lines) for the shell: list windows, workspace ops, screenshot requests, theme gamma. |

Config: `/etc/materialized/matcomp.toml` + user overrides in
`~/.config/materialized/matcomp.toml`, hot-reloaded via `SIGHUP`/inotify.

### 4.4 `matsessiond` — session daemon

Single owner of *appearance state*. Responsibilities:

1. Watch the wallpaper; decode it (Qt Image / libspng), downscale to ≤ 128×128,
   run `QuantizerCelebi` + `Score` → seed colours.
2. Build light + dark `DynamicScheme` for the user's variant/contrast choice.
3. Emit artifacts atomically (write to `.tmp`, `rename`) to
   `/run/user/$UID/materialized/theme/`:
   `scheme-light.json`, `scheme-dark.json`, `theme.qml`, `gtk.css`,
   `qt-palette.conf`, `xrdb`, `cursor.conf`, plus a `GTK_THEME`/`QT_QPA_PLATFORMTHEME`
   environment snippet imported by the session.
4. Publish `org.materialized.Theme1` (see §8) and push updates to all clients.
5. Own power/idle: idle timers (dim → lock → suspend) via `org.freedesktop.login1`
   and `ext-idle-notify-v1`; Night Light schedule (sunrise/sunset via geoclue or
   manual), driving the compositor gamma LUT.

### 4.5 `matshell` — the QML shell

One process, several Wayland surfaces (each a layer-shell window):

| Surface | Layer | Anchors | Content |
|---|---|---|---|
| `panel` | `top` | top/left/right | clock, workspaces, tray, quick-settings & notification buttons |
| `launcher` | `overlay` | none (exclusive fullscreen-ish) | app grid + fuzzy search + results |
| `quicksettings` | `overlay` | top/right | brightness/volume/night light/wifi/bt/airplane/power tiles |
| `notifications` | `overlay` | top/right | notification list, grouping, actions, history |
| `osd` | `overlay` | bottom/center | volume/brightness/caps-lock/keyboard-layout popups |
| `overview` | `overlay` | all | workspace switcher with live window thumbnails |
| `desktop` | `background` | all | wallpaper (owned by `matwallpaper` process) |

The shell talks to `matsessiond` (theme, power) and to `matcomp` (windows,
workspaces, screenshots) — never directly to hardware. It implements
`org.freedesktop.Notifications` and `org.freedesktop.impl.portal.*` is provided by
the separate portal process.

### 4.6 Applications

All apps share: `libm3-qt` components, `libmatdbus`, a common window chrome
(`MatWindow` = top app bar + optional nav rail + adaptive breakpoints
`compact <600dp ≤ medium <840dp ≤ expanded`), and consistent `.desktop`/icons.

| App | UI | Backend | Notes |
|---|---|---|---|
| `matfiles` | QML | Qt6 Core models + GIO for mounts (via `libmatdbus`/UDisks2), thumbnails via Qt ImageFormats | grid/list, tabs, split view, breadcrumbs, trash, network mounts, M3 context menus |
| `matsettings` | QML (nav rail + adaptive) | D-Bus: NM, BlueZ, UPower, UDisks2, logind, timedated, localed, hostnamed, PackageKit, matsessiond | every page is a real controller (`SettingsDisplay.cpp` etc.) |
| `matterminal` | GTK4 + VTE | `libvte-2.91-gtk4`, PTY, generated M3 GTK CSS | documented exception (D-022) |
| `mattext` | QML + QTextDocument | syntax highlighters, LSP client (optional), file watching | |
| `matsysmon` | QML + scene-graph graphs | `/proc`, `/sys`, systemd D-Bus, `getrusage` | per-process list, kill/renice via polkit |
| `matstore` | QML | PackageKit-Qt6 (APT), AppStream, Flatpak remote (optional) | curated M3 + GTK apps, updates page |
| `matphoto` | QML | Qt ImageFormats + libheif/libraw, EXIF | zoom, rotate, crop, set-as-wallpaper |
| `matscreenshot` | QML overlay + portal | `ext-image-copy-capture` / `wlr-screencopy` | region/window/fullscreen, timer, annotate, copy/save |
| `matcalc` | QML | expression parser (shunting-yard) + history tape | unit/currency conversion hooks |
| `matcalendar` | QML | local ICS store + CalDAV (Qt WebDAV), `org.freedesktop.timedate1` | month/week/agenda, reminders via notifications |
| `matmusic` | QML | TagLib metadata, QtMultimedia/GStreamer/PipeWire playback, MPRIS2 server | library, playlists, art-driven dynamic colour |
| `matweb` | Firefox ESR | packaged + generated `userChrome.css` from tokens, branded prefs | D-020 |

### 4.7 Installer

```
matinstall (QML wizard)  ──D-Bus──▶  matinstall-core (C++ engine, root via pkexec)
   pages: welcome · language · keyboard · timezone · disk (auto/manual) ·
          user · summary · progress
   engine steps (each: prepare/execute/rollback, JSON progress events):
     1 probe     → lsblk/UDisks2, detect existing OS for os-prober
     2 partition → sfdisk script (GPT: ESP, bios_grub, root, [home], swap)
     3 format    → mkfs.vfat / mkfs.ext4|mkfs.btrfs (subvolumes @ @home @snapshots)
     4 deploy    → unsquashfs of live rootfs (or debootstrap from pinned snapshot)
     5 configure → chroot: fstab, crypttab, hostname, locale, keymap, timezone,
                   users (useradd + passwd), groups, initramfs, GRUB install,
                   enable presets, remove live-only packages
     6 finalize  → verify, write install log, offer reboot
```

The engine is UI-agnostic: `matinstall-core --script answer.json` performs a
fully unattended install (used by the QEMU test harness for automated
install-and-boot validation).

---

## 5. Design-system data flow

```
design/tokens/*.json  ──────────────┐
                                    ▼
  wallpaper.png ─▶ decode ─▶ QuantizerCelebi ─▶ Score ─▶ seed ARGB
                                                          │
                       user prefs (variant, contrast, dark, accent override)
                                                          ▼
                                                    libm3 DynamicScheme
                                                          │
        ┌──────────────┬──────────────┬───────────────┬───┴──────────┬────────────┐
        ▼              ▼              ▼               ▼              ▼            ▼
   scheme.json     theme.qml      gtk.css        qt-palette.conf   xrdb       icons/tint
        │              │              │               │              │
        ▼              ▼              ▼               ▼              ▼
   libm3-qt        QML apps      GTK4 apps        Qt widgets     XWayland
   (M3Theme)       + matshell    + matterminal    (fallback)     legacy apps
```

**Two colour propagation paths:**

* **Live (fast):** `M3Theme` in each QML process subscribes to
  `org.materialized.Theme1.SchemeChanged`; on signal it re-reads
  `/run/user/$UID/materialized/theme/scheme-<mode>.json` (already written) and
  re-binds every role. Because all components bind to `M3Theme.*`, the whole
  desktop recolours within one frame — including already-open apps.
* **Static (compat):** generated `gtk.css`/`qt-palette.conf`/`xrdb` for toolkits
  that cannot subscribe. `GTK_THEME` is re-exported into the systemd user
  environment; running GTK apps are asked to reload via the
  `org.freedesktop.portal.Settings` colour-scheme signal where supported.

No process may invent colours. A component that needs a role asks for it by name
(`M3Theme.secondaryContainer`). The role table is generated, and the generator
fails the build if a token references an undefined role.

---

## 6. Graphics and rendering pipeline

```
app (QQuickWindow) ──wl_surface──▶ matcomp
   buffer: wl_drm/dmabuf (GLES) ─────────────┐
                                             ▼
                       wlr_scene_graph (nodes: surface, subsurface, rect, buffer)
                                             │  damage tracking per output
                                             ▼
                       wlr_renderer (GLES2 / Vulkan) → wlr_output → DRM/KMS pageflip
```

* **Rounded corners:** drawn by the compositor as a scene-graph rect node with a
  shader-based radius (server-side decorations), so *every* window — including
  XWayland and non-M3 apps — gets consistent 12 dp corners and a matching shadow.
  Clients that set `xdg-decoration: server-side` get the full M3 titlebar.
* **Damage:** wlroots computes the damaged region from buffer age + surface
  damage; we additionally invalidate on scene-graph node changes only. This is
  what keeps idle power low (§13 budget).
* **Direct scanout:** attempted for fullscreen single-surface views (video,
  games) — no composition pass at all.
* **Variable refresh:** `adaptive_sync` per output when the monitor supports it.
* **HDR/colour:** output ICC + `wp_color_management` when wlroots exposes it;
  tone-mapping path documented in `docs/specs/colour-management.md` (roadmap P7).

---

## 7. Session lifecycle (firmware → desktop)

```
firmware (UEFI or BIOS)
  └─ GRUB 2 ─▶ /vmlinuz root=UUID=… rw quiet splash mitigations=auto
      └─ systemd PID1 ─▶ multi-user.target
           ├─ getty@tty1..6 (text fallback, always available)
           ├─ matcomp.service  (systemd service, seat0, User=…)  ◀── starts on
           │      └─ wayland-session.target
           │           ├─ matsessiond.service (After=matcomp, WantedBy=session)
           │           ├─ xdg-desktop-portal-materialized.service
           │           ├─ matshell.service  (After=matsessiond)
           │           ├─ pipewire.service / wireplumber / pipewire-pulse
           │           ├─ NetworkManager, bluez (system bus, already up)
           │           └─ mat-polkit-agent.service
           └─ display-manager: none. We boot straight to the compositor on the
              primary seat (no GDM/SDDM). `matcomp` does the session-lock and
              user-switch itself; a greeter mode (`matcomp --greeter`) can be
              enabled for multi-user machines.
```

Boot budget target (SSD, mid-range x86_64): kernel→compositor **< 2.0 s**,
compositor→interactive shell **< 1.2 s**, cold boot to usable desktop **< 8 s**.
Enforced by `tests/vm/boot-time.py` reading `systemd-analyze blame`.

---

## 8. IPC contracts

### 8.1 D-Bus — `org.materialized.Theme1` (session bus)

```
/interface org.materialized.Theme1
  property Scheme      (a{sv})   # current resolved scheme: role → "#RRGGBBAA" + token metadata
  property IsDark      (b)
  property Variant     (s)       # tonal-spot|expressive|fidelity|vibrant|monochrome|content|neutral
  property Contrast    (d)       # -1.0 … 1.0  (default/medium/high)
  property AccentMode  (s)       # wallpaper|fixed|preset
  property AccentColor (u)       # ARGB when fixed
  method   SetMode(s mode)       # light|dark|auto
  method   SetVariant(s variant)
  method   SetContrast(d level)
  method   SetAccent(sv spec)    # {mode, argb}
  method   SetWallpaper(s path, s fit)   # triggers re-quantization
  method   Preview(scheme a{sv}) # transient, auto-reverts after 30 s
  signal   SchemeChanged(a{sv} scheme, s themePath)
  signal   WallpaperChanged(s uri, a{sv} meta)
```

### 8.2 D-Bus — `org.materialized.Shell1`

```
  method  ToggleLauncher() / ToggleQuickSettings() / ToggleOverview()
  method  ShowOsd(a{sv})            # {kind: volume|brightness|capslock, value, icon}
  method  Notify(a{sv} hints)       # convenience wrapper over fdo.Notifications
  method  Screenshot(a{sv} opts) → (s path)
  signal  PanelVisibilityChanged(b)
  property Workspaces (aa{sv})      # mirrored from compositor for tray applets
```

### 8.3 D-Bus — `org.materialized.Installer1` (system bus, polkit-gated)

```
  method  Probe() → (a{sv} disks)
  method  Plan(a{sv} answers) → (a{sv} plan)   # dry run, returns partition script
  method  Run(a{sv} answers) → (s jobId)
  method  Cancel(s jobId)
  signal  Progress(s jobId, s step, d fraction, s message)
  signal  Log(s jobId, s level, s text)
  signal  Finished(s jobId, b success, s reportPath)
```

### 8.4 Compositor control socket — `/run/user/$UID/matcomp.sock`

Newline-delimited JSON. Requests: `{"op":"workspace.switch","id":2}`,
`{"op":"window.list"}`, `{"op":"window.snap","id":…,"zone":"right"}`,
`{"op":"output.list"}`, `{"op":"gamma.set","output":"DP-1","lut":…}`,
`{"op":"capture","region":[x,y,w,h],"format":"argb"}`. This keeps the shell
free of privileged operations while avoiding a D-Bus round-trip per frame event.

### 8.5 File contracts

| Path | Producer | Consumers |
|---|---|---|
| `/run/user/$UID/materialized/theme/scheme-{light,dark}.json` | matsessiond | libm3-qt, apps, installer |
| `/run/user/$UID/materialized/theme/theme.qml` | matsessiond | QML singleton loader |
| `/run/user/$UID/materialized/theme/gtk.css` | matsessiond | GTK4/3 apps |
| `/run/user/$UID/materialized/theme/xrdb` | matsessiond | XWayland apps |
| `/etc/materialized/matcomp.toml`, `~/.config/materialized/matcomp.toml` | admin/user | matcomp |
| `/var/log/materialized/install-<ts>.json` | matinstall-core | support/debugging |

---

## 9. Input pipeline

```
evdev ─▶ libinput ─▶ wlr_input_device ─▶ matcomp InputManager
   ├─ keyboard: XKB keymap from localed/localectl → key bindings (toml, layered:
   │            default < user < app grab) → shell socket ops or compositor actions
   ├─ pointer:  focus follows mouse (configurable), pointer constraints for
   │            games, relative pointer for raw input, cursor theme from tokens
   ├─ touch:    tap-to-click, edge swipe from left → launcher, top-down → quick settings
   ├─ gestures: 3-finger horizontal → workspace; 3-finger up → overview;
   │            4-finger down → show desktop (libinput gesture API)
   └─ tablet:   wlr_tablet_v2 (absolute mapping, tilt/pressure passthrough)
```

Accessibility input features live here, not in apps: sticky keys, slow keys,
bounce keys, mouse keys (numpad → pointer), screen-reader hooks
(`org.a11y.Bus` + AT-SPI2 bridge), on-screen keyboard
(`text-input-v3` + `input-method-v2`, provided by `matkeyboard`).

---

## 10. Window management

* **Placement:** smart cascade on first window, center for dialogs.
* **Snap zones:** drag to edge → half; corner → quarter; `Super+Arrows` for
  keyboard snapping; a 2 s hover preview drawn with the M3 `primary` at 20 %
  alpha. Tiling mode (per-workspace toggle) gives master-stack or equal columns
  with gap = `tokens.spacing.md` (12 dp).
* **Workspaces:** `ext-workspace-unstable-v1`, 1–9 per output group, window
  thumbnails via `screencopy` at 1 Hz while the overview is open.
* **Focus order:** MRU stack; `Alt+Tab` switcher rendered by the shell.
* **Decorations:** server-side by default (`xdg-decoration: server-side`
  negotiated); titlebar = `surface` with `titleLarge` text, 12 dp radius,
  M3 elevation shadow baked as a scene-graph buffer. Clients that insist on CSD
  (GTK) get their own and are excluded from server rounding.

---

## 11. Notification pipeline

```
app ─org.freedesktop.Notifications─▶ matshell (NotificationServer)
   ├─ store: ring buffer (500) in ~/.local/share/materialized/notifications.db (SQLite)
   ├─ grouping: app-id + summary similarity, collapsed to a "n more" card
   ├─ urgency: critical → OSD banner + sound (libcanberra/event-sound), normal → panel badge
   ├─ actions: invoked back over the same bus; inline replies supported
   └─ persistence across restarts: history view in the notification center
```

Do-Not-Disturb and per-app rules live in `matsessiond` (they must survive shell
restarts) and are pushed to `matshell`.

---

## 12. Power, idle, lock

* `matsessiond` owns an idle state machine:
  `active → idle(dim @ N-120 s) → lock(@ N-60 s) → dpms-off(@ N) → suspend(@ N+M)`
  driven by `ext-idle-notify-v1` from the compositor and `login1` inhibitors.
* Lock: `ext-session-lock-v1` → compositor grabs input and hands each output to
  `matlock`; authentication via PAM (`materialized-lock` service, `pam_unix` +
  optional `pam_fprintd`/`pam_systemd_home`). The lock surface is created
  *before* the grab is acknowledged, so there is no visible unlock hole.
* Lid switch / power button / suspend-then-hibernate: `logind.conf` shipped in
  `platform/configs/`, overridable in Settings → Power.
* Battery: UPower; the panel shows a M3-style battery glyph and time-remaining;
  power profiles (`power-profiles-daemon`) exposed as a quick-setting tile.

---

## 13. Performance budget

| Metric | Target | Measured by |
|---|---|---|
| Idle RAM (compositor + shell + sessiond) | ≤ 380 MB RSS | `tests/vm/mem-report.py` |
| Idle CPU (desktop visible, nothing running) | ≤ 0.5 % of one core | same |
| Frame time under load (dragging a window) | ≤ 8 ms p95 on iGPU | wlroots `WLR_DEBUG` timing + `matcomp --stats` |
| Cold boot to desktop | ≤ 8 s | `systemd-analyze` |
| Cold launch of a QML app | ≤ 350 ms | `matapp-launch-bench` |
| Theme switch (wallpaper → all apps recoloured) | ≤ 120 ms | `tests/unit/m3/theme-latency.cpp` |
| ISO size | ≤ 2.6 GiB | build log |
| Installed size | ≤ 6.5 GiB | `du -sx /` |

Techniques: QML disk cache (`qmlcachegen` at build time), `qt.qpa` minimal
platform plugins, no `QtWebEngine` anywhere in the shell, compositor-side
direct scanout, `zram` swap (50 % of RAM, lz4) instead of a swap partition on
low-RAM machines, `systemd` service `Slice=` + `MemoryHigh` on non-critical
daemons, and lazy app-startup (`matstore` metadata refresh is timer-gated).

---

## 14. Packaging and image build

```
source tree ──dpkg-buildpackage──▶ .deb per component
   meta packages: mat-core (kernel, systemd, base utils)
                  mat-desktop (matcomp, matshell, matsessiond, libm3-qt, portal)
                  mat-apps (the 12 applications)
                  mat-live (live-session bits, casper-free: our own initramfs hook)
   local APT repo: apt-ftparchive + Release signing (minisign/gpg) in build/repo/

platform/iso/build.sh
   1. debootstrap --variant=minbase trixie $ROOT http://deb.debian.org/debian (pinned snapshot)
   2. chroot: apt install mat-core mat-desktop mat-apps (+ firmware, drivers, fonts)
   3. live customisation: create 'materialized' live user, autologin on tty1 → matcomp
   4. mksquashfs $ROOT → filesystem.squashfs (xz, 1 MiB blocks)
   5. initramfs hook: overlay root (squashfs ro + tmpfs rw), persistence optional
   6. xorriso: hybrid ISO with GRUB (_eltorito EFI img + i386-pc boot.img), isolinux-free
   7. checksum + zsync; publish build manifest (package list, versions, hashes)
```

Reproducibility: `SOURCE_DATE_EPOCH` honoured everywhere, `dpkg-buildpackage`
with `-j1` and pinned `--build-profiles`, debootstrap against
`snapshot.debian.org` at a fixed timestamp recorded in `platform/iso/snapshot.txt`.

---

## 15. Security model

* Compositor runs as the user on `seat0`, no setuid; DRM master via logind.
* Installer core is the only privileged helper; polkit rule requires
  `org.materialized.install` in `wheel`/`sudo` group with auth_admin_keep.
* Apps are unsandboxed by default (Debian semantics) but `matstore` can install
  Flatpak with `--user` and the portal mediates file access for sandboxed apps.
* Secrets (Wi-Fi PSK, CalDAV creds) go to `libsecret`/GNOME Keyring via
  Secret Service, never to our own files.
* Lock screen: separate PAM service, no root helpers, screenshot disabled while
  locked, `ext-session-lock` refuses to release the lock until PAM succeeds.
* No telemetry. Crash reports are opt-in and stay local
  (`/var/lib/materialized/crash`, processed by `matreport`).

---

## 16. Testing architecture

| Layer | Tooling | Runs where |
|---|---|---|
| Unit (pure logic) | Catch2 + QtTest: libm3 (fixture-verified ARGB), token generator, calculator parser, partition planner, installer step machine | any host, incl. WSL2/Windows CI |
| Component | compositor with wlroots **headless backend**; shell driven by a test harness that injects synthetic Wayland events | Linux only |
| Integration | QEMU (`tests/vm/run-qemu.sh`): boot ISO (UEFI + BIOS), automated install via `matinstall-core --script`, boot installed system, screenshot diff against golden images | Linux/KVM or WSL2 with nested virt |
| Visual regression | headless compositor + `screencopy` → PNG → perceptual diff (`tests/vm/visual-diff.py`, CIEDE2000 threshold) | CI |
| Performance | §13 budgets, asserted in `tests/vm/perf.py`, failures block release | CI |
| Accessibility | contrast audit (`tools/a11y-contrast.py` checks every role pair ≥ 4.5:1 for text roles) + keyboard-only navigation autopilot | CI |

---

## 17. Extension points

* **New theme variant:** add a `Scheme*` subclass in `libm3/scheme/`, register in
  `variant_registry.cpp`, add its name to `design/tokens/colour-roles.json`
  allow-list; everything downstream picks it up automatically.
* **New shell applet:** a QML component in `shell/matshell/qml/applets/` plus an
  entry in `applets.json` — the panel instantiates it; no C++ rebuild needed.
* **New app:** `cmake -P scaffolding/new-app.cmake NAME matfoo` generates a
  deb-ready tree wired to `libm3-qt` and `libmatdbus`.
* **Alternative compositor:** the shell depends only on layer-shell +
  `ext-workspace` + our control socket, so it can run on other wlroots
  compositors (`matshell --compat=sway`) for development without `matcomp`.

---

## 18. Open architectural risks

| Risk | Mitigation | Owner doc |
|---|---|---|
| wlroots API churn (0.18 → 0.19 renamed renderer/scene APIs) | pin exact tag in `third_party/manifest.json`, wrap in `compositor/matcomp/wlr_compat.hh` | ROADMAP P2 |
| QML shell startup cost on 4 GB machines | `qmlcachegen`, single process for all shell surfaces, lazy applet loading | §13 |
| GTK apps not honouring generated CSS (GTK4 CSD, libadwaita overrides) | ship a small `libadwaita` colour override + document the 10 % of apps that will look off | docs/specs/third-party-theming.md |
| Wayland-only screenshots vs. X11 apps | XWayland + portal fallback path | §4.6 matscreenshot |
| Debian trixie Qt 6.8 vs. newer Qt features | baseline = Qt 6.8; anything newer is behind `MAT_QT_NEXT` guards | BUILDING.md |
