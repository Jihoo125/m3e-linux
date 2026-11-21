#!/usr/bin/env bash
# build/fix-wsl-clock.sh — repair WSL2 clock drift (common after host sleep/hibernate).
#
# WSL2's clock can lag hours behind real time; apt then rejects every Release file
# as "not valid yet". This resynchronises using an HTTP Date header (no NTP needed).
set -u

if [ "$(id -u)" -ne 0 ]; then
  echo "must run as root (wsl.exe -d Ubuntu -u root -- bash build/fix-wsl-clock.sh)" >&2
  exit 1
fi

CANDIDATE_URLS=(
  "http://archive.ubuntu.com/ubuntu/dists/jammy/InRelease"
  "https://deb.debian.org/debian/dists/trixie/InRelease"
  "https://www.google.com/"
)

NET_DATE=""
for url in "${CANDIDATE_URLS[@]}"; do
  if command -v curl >/dev/null 2>&1; then
    NET_DATE=$(curl -sI --max-time 10 "$url" | sed -n 's/^[Dd]ate:[[:space:]]*//p' | tr -d '\r' | head -1)
  elif command -v wget >/dev/null 2>&1; then
    NET_DATE=$(wget -S --spider --timeout=10 "$url" 2>&1 | sed -n 's/^.*[Dd]ate:[[:space:]]*//p' | tr -d '\r' | head -1)
  fi
  if [ -n "$NET_DATE" ]; then
    echo "authoritative date from $url:"
    echo "  $NET_DATE"
    break
  fi
done

if [ -z "$NET_DATE" ]; then
  echo "could not determine network time (no curl/wget or no network)" >&2
  echo "manual fallback:  date -s '2026-11-13 12:00:00'" >&2
  exit 1
fi

BEFORE=$(date -u '+%Y-%m-%d %H:%M:%S UTC')
if date -s "$NET_DATE" >/dev/null 2>&1; then
  AFTER=$(date -u '+%Y-%m-%d %H:%M:%S UTC')
  echo "clock before: $BEFORE"
  echo "clock after:  $AFTER"
  # hwclock keeps it across the WSL session on newer WSL versions.
  hwclock -w >/dev/null 2>&1 || true
  echo "ok"
else
  echo "failed to set clock" >&2
  exit 1
fi
