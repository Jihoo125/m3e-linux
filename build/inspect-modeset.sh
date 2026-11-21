#!/usr/bin/env bash
set -euo pipefail
cd /var/tmp
printf '== modeset commit function ==\n'
grep -n -A55 'drm_connector_commit_modeset' wlr_drm.c | head -80 || true
printf '\n== atomic_crtc_commit / modeset flags ==\n'
grep -n -B3 -A30 'atomic_modeset\|atomic_crtc_commit\|DRM_MODE_PAGE_FLIP_EVENT\|DRM_MODE_ATOMIC_ALLOW_MODESET' wlr_atomic.c | head -120 || true
printf '\n== where send_frame called in drm ==\n'
grep -n 'send_frame\|frame_pending\|pending_page_flip' wlr_drm.c || true
