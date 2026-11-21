#!/usr/bin/env bash
set -euo pipefail
cd /var/tmp
printf '== drm.c atomic flags ==\n'
grep -n -B10 -A25 'DRM_MODE_ATOMIC_NONBLOCK' wlr_drm.c | head -90 || true
printf '\n== drm_connector_commit ==\n'
grep -n -A60 'static bool drm_connector_commit' wlr_drm.c | head -90 || true
printf '\n== frame_pending assignments ==\n'
grep -n 'frame_pending' wlr_drm.c || true
