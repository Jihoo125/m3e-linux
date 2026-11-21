#!/usr/bin/env bash
# Refresh Debian archive trust using Debian's signed current keyring package.
# No signature checking is disabled and no unverified key is accepted silently.
set -euo pipefail
if [ "$(id -u)" -ne 0 ]; then exec sudo -E bash "$0" "$@"; fi
export DEBIAN_FRONTEND=noninteractive
WORKDIR=/var/tmp/materialized-debian-trust
mkdir -p "$WORKDIR"
cd "$WORKDIR"

# Ubuntu 22.04's keyring predates the current Debian keys. Debian 13 supplies a
# newer signed package. Read Release metadata rather than hard-coding filenames.
curl --proto '=https' --tlsv1.2 -fsSL https://deb.debian.org/debian/dists/trixie/Release -o Release
curl --proto '=https' --tlsv1.2 -fsSL https://deb.debian.org/debian/dists/trixie/Release.gpg -o Release.gpg
curl --proto '=https' --tlsv1.2 -fsSL https://deb.debian.org/debian/dists/trixie/main/binary-amd64/Packages.xz -o Packages.xz

python3 - <<'PY'
from pathlib import Path
import lzma
packages = lzma.open('Packages.xz', 'rt', encoding='utf-8')
for stanza in packages.read().split('\n\n'):
    if stanza.startswith('Package: debian-archive-keyring\n'):
        fields = {}
        for line in stanza.splitlines():
            if ': ' in line:
                key, value = line.split(': ', 1)
                fields[key] = value
        print(fields['Filename'])
        print(fields['SHA256'])
        break
else:
    raise SystemExit('debian-archive-keyring not found in trixie Packages')
PY

FILENAME=$(python3 - <<'PY'
import lzma
for stanza in lzma.open('Packages.xz', 'rt', encoding='utf-8').read().split('\n\n'):
    if stanza.startswith('Package: debian-archive-keyring\n'):
        for line in stanza.splitlines():
            if line.startswith('Filename: '): print(line[10:])
PY
)
EXPECTED=$(python3 - <<'PY'
import lzma
for stanza in lzma.open('Packages.xz', 'rt', encoding='utf-8').read().split('\n\n'):
    if stanza.startswith('Package: debian-archive-keyring\n'):
        for line in stanza.splitlines():
            if line.startswith('SHA256: '): print(line[8:])
PY
)
curl --proto '=https' --tlsv1.2 -fsSL "https://deb.debian.org/debian/$FILENAME" -o keyring.deb
printf '%s  %s\n' "$EXPECTED" keyring.deb | sha256sum -c -
ar x keyring.deb
tar -xJf data.tar.xz ./usr/share/keyrings/debian-archive-keyring.gpg

# Debian distributes the stable release key separately from the archive keys.
curl --proto '=https' --tlsv1.2 -fsSL https://ftp-master.debian.org/keys/archive-key-12.asc -o archive-key-12.asc
curl --proto '=https' --tlsv1.2 -fsSL https://ftp-master.debian.org/keys/archive-key-12-security.asc -o archive-key-12-security.asc
curl --proto '=https' --tlsv1.2 -fsSL https://ftp-master.debian.org/keys/archive-key-13.asc -o archive-key-13.asc
curl --proto '=https' --tlsv1.2 -fsSL https://ftp-master.debian.org/keys/archive-key-13-security.asc -o archive-key-13-security.asc
curl --proto '=https' --tlsv1.2 -fsSL https://ftp-master.debian.org/keys/release-13.asc -o release-13.asc
EXPECTED_RELEASE_FP='41587F7DB8C774BCCF131416762F67A0B2C39DE4'
gpg --no-default-keyring --keyring ./usr/share/keyrings/debian-archive-keyring.gpg --import archive-key-12.asc archive-key-12-security.asc archive-key-13.asc archive-key-13-security.asc release-13.asc
ACTUAL=$(gpg --no-default-keyring --keyring ./usr/share/keyrings/debian-archive-keyring.gpg \
    --with-colons --fingerprint 2>/dev/null | awk -F: '$1=="fpr" && $10=="'$EXPECTED_RELEASE_FP'" { print $10; exit }')
if [ "$ACTUAL" != "$EXPECTED_RELEASE_FP" ]; then
    echo "Debian 13 stable signing key fingerprint mismatch: $ACTUAL" >&2
    exit 1
fi
if ! gpgv --keyring ./usr/share/keyrings/debian-archive-keyring.gpg Release.gpg Release; then
    echo "Could not verify the signed trixie Release file" >&2
    exit 1
fi
mkdir -p /usr/local/share/keyrings
install -m 0644 ./usr/share/keyrings/debian-archive-keyring.gpg /usr/local/share/keyrings/debian-archive-keyring-current.gpg
printf '\nVerified Debian keyring installed. ISO builds use:\n'
printf '  DEBOOTSTRAP_KEYRING=/usr/local/share/keyrings/debian-archive-keyring-current.gpg\n'
