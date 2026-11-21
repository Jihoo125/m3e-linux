#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/iris8/Downloads/yes
if [ ! -d .git ]; then
    git init -b main
fi
git config user.name "Qwen4-Max wooo"
git config user.email "jihoo125@j1t.xyz"
git remote remove origin 2>/dev/null || true
git remote add origin https://github.com/Jihoo125/m3e-linux.git
git add -A
git commit -m "Materialized Linux 0.2.0 (Alpha 0.2.0)

Real M3 Expressive (spring motion, shape morph, emphasized type, variant),
9 apps incl. PTY terminal, image viewer, screencopy screenshot, calendar,
Firefox ESR with generated Material chrome, XDG autostart, launch hotkeys.
Verified: UEFI+BIOS QEMU boot to exposed QML shell with apps running.
ISO SHA-256: c7956d6895a15d6449f14e6e73b22b0c7549f31e54400458ad087b518b10a574"
git tag -f v0.2.0
printf '\n== commit ==\n'
git log --oneline -1
git show -s --format='author: %an <%ae>%ncommitter: %cn <%ce>'
printf '\n== tracked file count ==\n'
git ls-files | wc -l
printf '\n== push ==\n'
GIT_TERMINAL_PROMPT=0 git push -u origin main 2>&1 || printf 'PUSH_FAILED\n'
