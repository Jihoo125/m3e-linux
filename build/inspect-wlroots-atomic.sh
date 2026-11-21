#!/usr/bin/env bash
set -euo pipefail
T=/var/tmp/wlroots-0.15.1.tar.gz
if [ ! -s "$T" ]; then
    curl -fsSL https://gitlab.freedesktop.org/wlroots/wlroots/-/archive/0.15.1/wlroots-0.15.1.tar.gz -o "$T"
fi
cd /var/tmp
tar -xOf "$T" wlroots-0.15.1/backend/drm/atomic.c > wlr_atomic.c
tar -xOf "$T" wlroots-0.15.1/render/allocator/allocator.c > wlr_allocator.c
printf '== atomic page-flip flags ==\n'
grep -n -B6 -A10 'PAGE_FLIP_EVENT' wlr_atomic.c | head -60 || true
printf '\n== atomic commit function ==\n'
grep -n -A45 'bool atomic_commit' wlr_atomic.c | head -70 || true
printf '\n== allocator autocreate ==\n'
grep -n -A40 'wlr_allocator_autocreate' wlr_allocator.c | head -60 || true
printf '\n== send_frame ==\n'
grep -n -A16 'void wlr_output_send_frame' wlr_output.c || true
printf '\n== drm modeset commit ==\n'
grep -n -B4 -A30 'drm_connector_commit_modeset' wlr_drm.c | head -80 || true
