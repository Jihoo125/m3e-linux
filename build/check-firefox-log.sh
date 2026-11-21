#!/usr/bin/env bash
set -euo pipefail
LOG=/mnt/c/Users/iris8/Downloads/yes/build/vm/boot-uefi.log
printf '== firefox/browser mentions ==\n'
grep -a -iE 'firefox|matbrowser|MOZ|sandbox|namespace' "$LOG" | tail -25 || true
printf '\n== last 30 lines ==\n'
tail -30 "$LOG"
