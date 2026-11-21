#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/iris8/Downloads/yes
# Remove 0-byte junk created by earlier shell redirection accidents.
[ -f '&1' ] && [ ! -s '&1' ] && rm -- '&1'
[ -f Test ] && [ ! -s Test ] && rm Test
# Vendor the MCU reference sources: the inner clone carries a local commit,
# so a submodule gitlink would not resolve for other users. Keep the original
# clone metadata as .git-upstream (ignored) instead of deleting it.
MCU=third_party/src/material-color-utilities
if [ -d "$MCU/.git" ]; then
    mv "$MCU/.git" "$MCU/.git-upstream"
fi
git rm -r --cached -q "$MCU" 2>/dev/null || true
git rm -r --cached -q tests/unit/__pycache__ tools/__pycache__ 2>/dev/null || true
git rm --cached -q -- '&1' Test 2>/dev/null || true
git add -A
git -c user.name="Qwen4-Max wooo" -c user.email="jihoo125@j1t.xyz" commit --amend --no-edit --reset-author
git tag -f v0.2.0
git ls-files | wc -l
git log --oneline -1
