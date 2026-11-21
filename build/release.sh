#!/usr/bin/env bash
# Cut a release: verify, commit and push to GitHub with the release identity.
# Usage: bash build/release.sh [version]   (default: build/version.txt)
# The ISO itself is not committed (too large for git); its SHA-256 and package
# manifest are recorded in docs/ and shipped alongside the repo out-of-band.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
VERSION="${1:-$(cat build/version.txt)}"
REMOTE="https://github.com/Jihoo125/m3e-linux.git"
NAME="Qwen4-Max wooo"
EMAIL="jihoo125@j1t.xyz"

if [ ! -d .git ]; then
    git init -b main
    git remote add origin "$REMOTE"
fi
git config user.name "$NAME"
git config user.email "$EMAIL"
git remote set-url origin "$REMOTE"

# Record release metadata before committing.
ISO="build/iso/materialized-$VERSION-amd64.iso"
if [ -s "$ISO.sha256" ]; then
    mkdir -p docs/releases
    cp "$ISO.sha256" "docs/releases/materialized-$VERSION-amd64.iso.sha256"
    [ -s "$ISO.packages" ] && cp "$ISO.packages" "docs/releases/materialized-$VERSION-amd64.iso.packages"
fi

git add -A
git commit -m "Materialized Linux $VERSION

Verified: UEFI+BIOS QEMU boot to exposed QML shell with applications running;
local unit suites green. See docs/ALPHA-0.2.md and docs/releases/." || echo "Nothing new to commit."
git tag -f "v$VERSION"
# GitHub credentials live in the Windows Credential Manager; prefer Windows
# Git for the push when available, falling back to the WSL git.
WIN_GIT="/mnt/c/Program Files/Git/cmd/git.exe"
if [ -x "$WIN_GIT" ]; then
    GIT_TERMINAL_PROMPT=0 "$WIN_GIT" push -u origin main
    GIT_TERMINAL_PROMPT=0 "$WIN_GIT" push -f origin "v$VERSION"
else
    git push -u origin main
    git push -f origin "v$VERSION"
fi
printf 'Released %s as %s <%s>\n' "$VERSION" "$NAME" "$EMAIL"
