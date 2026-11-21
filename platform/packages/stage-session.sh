#!/usr/bin/env bash
# Package the built Materialized session into a Linux staging root.
# This is deliberately not a privileged system install.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_ROOT="${MAT_BUILD_ROOT:-$ROOT/build}"
STAGING="${MAT_STAGING:-$ROOT/build/staging}"
rm -rf "$STAGING"
mkdir -p "$STAGING/usr/bin" "$STAGING/usr/share/applications" "$STAGING/usr/share/materialized/qml/Materialized" "$STAGING/usr/share/backgrounds/materialized" "$STAGING/usr/lib/systemd/user" "$STAGING/etc/materialized"

copy_binary() {
    local source="$1"
    local name="${2:-$(basename "$source")}"
    if [ ! -x "$source" ]; then echo "Missing binary: $source" >&2; exit 1; fi
    install -Dm755 "$source" "$STAGING/usr/bin/$name"
}

copy_binary "$BUILD_ROOT/matcomp/matcomp"
copy_binary "$BUILD_ROOT/matsessiond/matsessiond"
copy_binary "$BUILD_ROOT/matshell/matshell"
copy_binary "$BUILD_ROOT/qml/bin/matgallery"
for app in matcalc mattext matsysmon matfiles matsettings matterminal matview matshot matcal; do
    copy_binary "$BUILD_ROOT/$app/$app"
done
if [ -x "$BUILD_ROOT/installer-clean/matinstall-core" ]; then
    copy_binary "$BUILD_ROOT/installer-clean/matinstall-core"
fi

for file in "$ROOT"/platform/packages/desktop-entries/*.desktop; do
    install -Dm644 "$file" "$STAGING/usr/share/applications/$(basename "$file")"
done
install -Dm644 "$ROOT/design/wallpaper/materialized-dawn.png" "$STAGING/usr/share/backgrounds/materialized/materialized-dawn.png"
install -Dm644 "$ROOT/design/wallpaper/materialized-dawn.svg" "$STAGING/usr/share/backgrounds/materialized/materialized-dawn.svg"
install -Dm644 "$ROOT/compositor/matcomp/matcomp.toml" "$STAGING/etc/materialized/matcomp.toml"
# matcomp targets the wlroots 0.15 ABI. Include that ABI in the live image so it
# does not depend on the host's distro-specific wlroots soname.
if [ -f /usr/lib/x86_64-linux-gnu/libwlroots.so.10 ]; then
    install -Dm755 /usr/lib/x86_64-linux-gnu/libwlroots.so.10 "$STAGING/usr/lib/x86_64-linux-gnu/libwlroots.so.10"
fi
install -Dm644 "$BUILD_ROOT/qml/qml/Materialized/qmldir" "$STAGING/usr/share/materialized/qml/Materialized/qmldir"
# This is a filesystem QML module. A resource-only preference would make Qt
# look under qrc:/Materialized instead of these installed M3*.qml files.
sed -i '/^prefer[[:space:]]/d' "$STAGING/usr/share/materialized/qml/Materialized/qmldir"
if ! grep -q '^M3Button 1.0 M3Button.qml$' "$STAGING/usr/share/materialized/qml/Materialized/qmldir"; then
python3 - "$STAGING/usr/share/materialized/qml/Materialized/qmldir" "$ROOT/libs/libm3-qt/qml" <<'PY'
from pathlib import Path
import sys
path = Path(sys.argv[1])
content = path.read_text()
for qml in sorted(Path(sys.argv[2]).glob('M3*.qml')):
    content += f'{qml.stem} 1.0 {qml.name}\n'
path.write_text(content)
PY
fi
install -Dm755 "$BUILD_ROOT/qml/qml/Materialized/libm3plugin.so" "$STAGING/usr/share/materialized/qml/Materialized/libm3plugin.so"
for qml in "$ROOT"/libs/libm3-qt/qml/M3*.qml; do
    install -Dm644 "$qml" "$STAGING/usr/share/materialized/qml/Materialized/$(basename "$qml")"
done

install -Dm755 /dev/stdin "$STAGING/usr/bin/matbrowser" <<'EOF'
#!/bin/sh
# Real Firefox ESR with a Material 3 chrome generated from the live scheme.
set -eu
# Diagnostics: keep a guest log and flush it to the serial console on exit so
# VM acceptance can see startup failures even if Firefox never maps a window.
exec >>/tmp/matbrowser.log 2>&1
trap 'cat /tmp/matbrowser.log > /dev/ttyS0 2>/dev/null || true' EXIT
echo "matbrowser: starting firefox-esr"
# Native Wayland: the alpha compositor has no XWayland yet.
export MOZ_ENABLE_WAYLAND=1
export MOZ_DBUS_REMOTE=1
# Live VMs frequently restrict unprivileged user namespaces; the content
# sandbox cannot initialize there. Relax it for the evaluation session only.
export MOZ_DISABLE_CONTENT_SANDBOX=1
export MOZ_DISABLE_RDD_SANDBOX=1
export MOZ_DISABLE_SOCKET_PROCESS_SANDBOX=1
PROFILE="${XDG_DATA_HOME:-$HOME/.local/share}/materialized/firefox-profile"
mkdir -p "$PROFILE/chrome"
RUNTIME="${XDG_RUNTIME_DIR:-/tmp/materialized-$UID}"
THEME="$RUNTIME/materialized/theme"
if [ -f "$THEME/firefox-userChrome.css" ]; then
    cp "$THEME/firefox-userChrome.css" "$PROFILE/chrome/userChrome.css"
fi
cat > "$PROFILE/user.js" <<'PREFS'
user_pref("toolkit.legacyUserProfileCustomizations.stylesheets", true);
user_pref("browser.shell.checkDefaultBrowser", false);
user_pref("browser.startup.homepage_override.mstone", "ignore");
user_pref("startup.homepage_welcome_url", "about:blank");
user_pref("startup.homepage_welcome_url.additional", "about:blank");
user_pref("browser.rights.3.shown", true);
user_pref("datareporting.policy.dataSubmissionPolicyBypassNotification", true);
user_pref("browser.theme.content-theme", 1);
user_pref("browser.theme.toolbar-theme", 1);
user_pref("extensions.activeThemeID", "firefox-compact-dark@mozilla.org");
PREFS
exec firefox-esr --profile "$PROFILE" --no-remote "$@"
EOF

install -Dm644 /dev/stdin "$STAGING/etc/xdg/autostart/materialized-terminal.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Materialized Terminal
Exec=matterminal
OnlyShowIn=Materialized;
X-Materialized-Autostart=alpha-demo
EOF

install -Dm644 /dev/stdin "$STAGING/etc/xdg/autostart/materialized-browser.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Materialized Browser
Exec=matbrowser
OnlyShowIn=Materialized;
X-Materialized-Autostart=alpha-demo
EOF

install -Dm644 /dev/stdin "$STAGING/usr/lib/systemd/user/matsessiond.service" <<'EOF'
[Unit]
Description=Materialized appearance and wallpaper service
After=graphical-session.target
PartOf=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/matsessiond
Restart=on-failure
RestartSec=2

[Install]
WantedBy=graphical-session.target
EOF

install -Dm644 /dev/stdin "$STAGING/usr/lib/systemd/user/matshell.service" <<'EOF'
[Unit]
Description=Materialized desktop shell
After=matsessiond.service
Requires=matsessiond.service
PartOf=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/matshell
Restart=on-failure
RestartSec=2

[Install]
WantedBy=graphical-session.target
EOF

install -Dm755 /dev/stdin "$STAGING/usr/bin/materialized-session" <<'EOF'
#!/bin/sh
set -eu
: "${XDG_RUNTIME_DIR:=/tmp/materialized-$UID}"
export XDG_RUNTIME_DIR
mkdir -p "$XDG_RUNTIME_DIR/materialized/theme"
chmod 700 "$XDG_RUNTIME_DIR"
export QML2_IMPORT_PATH="${QML2_IMPORT_PATH:-/usr/share/materialized/qml}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland}"
export QT_QUICK_CONTROLS_STYLE="${QT_QUICK_CONTROLS_STYLE:-Basic}"
# The alpha compositor renders with pixman (software). Qt must submit shm
# buffers, not GPU dmabufs, or surfaces map but never appear on screen.
export QT_QUICK_BACKEND="${QT_QUICK_BACKEND:-software}"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"
# matcomp is the display server on the live TTY, not a nested Wayland client.
unset WAYLAND_DISPLAY DISPLAY
export WLR_BACKENDS="${WLR_BACKENDS:-drm,libinput}"
export WLR_RENDERER="${WLR_RENDERER:-pixman}"
export MAT_COMPOSITOR_CONFIG="${MAT_COMPOSITOR_CONFIG:-/etc/materialized/matcomp.toml}"
export MAT_SCHEME_PATH="$XDG_RUNTIME_DIR/materialized/theme/scheme-light.json"
export MAT_WALLPAPER=/usr/share/backgrounds/materialized/materialized-dawn.png
export DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus"
export XDG_CURRENT_DESKTOP=Materialized
export XDG_SESSION_DESKTOP=Materialized
export XDG_SESSION_TYPE=wayland
# A tty autologin is not a logind graphical session. Start a real session bus
# for notifications, settings and application activation before the shell.
if [ ! -S "$XDG_RUNTIME_DIR/bus" ]; then
    /usr/bin/dbus-daemon --session --fork --address="$DBUS_SESSION_BUS_ADDRESS" --print-pid=1 > "$XDG_RUNTIME_DIR/dbus.pid"
fi
/usr/bin/matcomp 2>&1 | tee /tmp/materialized-compositor.log > /dev/ttyS0 &
COMPOSITOR_PID=$!
# Independent watchdog: report session process state and app logs to serial
# every 5 s, so VM acceptance has ground truth even if a client misbehaves.
(
    while :; do
        sleep 5
        {
            echo "--- watchdog ---"
            ps -eo pid,comm | grep -E 'matcomp|matshell|matsessiond|firefox|matterminal|matbrowser' || true
            cat /tmp/matbrowser.log 2>/dev/null || true
        } > /dev/ttyS0 2>/dev/null || true
    done
) &
WATCHDOG_PID=$!
READY=0
for _ in $(seq 1 200); do
    if [ -S "$XDG_RUNTIME_DIR/wayland-0" ]; then READY=1; break; fi
    if ! kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
        echo "matcomp exited before creating $XDG_RUNTIME_DIR/wayland-0" >&2
        exit 1
    fi
    sleep 0.1
done
if [ "$READY" -ne 1 ]; then
    echo "Timed out waiting for the Materialized Wayland socket" >&2
    kill "$COMPOSITOR_PID" 2>/dev/null || true
    exit 1
fi
export WAYLAND_DISPLAY=wayland-0
/usr/bin/matsessiond --output "$XDG_RUNTIME_DIR/materialized/theme" --wallpaper "$MAT_WALLPAPER" &
SESSION_PID=$!
trap 'kill "$SESSION_PID" "$COMPOSITOR_PID" "$WATCHDOG_PID" 2>/dev/null || true' EXIT INT TERM
# Logs are copied to the serial console on exit so an autologin loop cannot
# hide a failing compositor or QML startup.
/usr/bin/matshell 2>&1 | tee /tmp/materialized-session.log > /dev/ttyS0
STATUS=${PIPESTATUS[0]:-$?}
exit "$STATUS"
EOF

# Generate an explicit runtime manifest: every binary is inspectable and hashed.
find "$STAGING" -type f -print0 | sort -z | xargs -0 sha256sum > "$STAGING/MANIFEST.sha256"
printf 'Staged session in %s\n' "$STAGING"
