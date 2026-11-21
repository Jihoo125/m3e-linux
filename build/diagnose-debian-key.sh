#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
mkdir -p "$ROOT/build/debian-verify"
cd "$ROOT/build/debian-verify"
curl -fsSLO http://deb.debian.org/debian/dists/trixie/InRelease
curl -fsSLO http://deb.debian.org/debian/dists/trixie/Release
curl -fsSLO http://deb.debian.org/debian/dists/trixie/Release.gpg
printf '\n== Signing key packets ==\n'
gpg --list-packets Release.gpg | grep -E 'signature packet|keyid|issuer fpr' | head -12
printf '\n== Available Debian keyrings ==\n'
ls -l /usr/share/keyrings/*debian* /usr/share/keyrings/*ubuntu* 2>/dev/null || true
printf '\n== Verification with installed keyring ==\n'
gpgv --keyring /usr/share/keyrings/debian-archive-keyring.gpg Release.gpg Release 2>&1 || true
printf '\n== Host time ==\n'
date -u
printf '\n== Network time ==\n'
curl -fsSI http://deb.debian.org/debian/dists/trixie/InRelease | sed -n 's/^[Dd]ate:[[:space:]]*//p' | tr -d '\r'
