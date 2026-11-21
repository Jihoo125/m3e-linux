#!/usr/bin/env bash
set -euo pipefail
LOG=/mnt/c/Users/iris8/Downloads/yes/build/vm/boot-uefi.log
printf '== watchdog blocks (last 40 lines of them) ==\n'
grep -a -A 12 -- '--- watchdog ---' "$LOG" | tail -60 || true
printf '\n== matbrowser/firefox mentions ==\n'
grep -a -nE 'matbrowser|firefox|launch' "$LOG" | tail -20 || true
