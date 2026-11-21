#!/usr/bin/env bash
# Install lightweight runtime pieces needed for development on Ubuntu/WSLg.
# libqt6svg6 is real SVG icon rendering; libqt6waylandclient6 is Qt's WSLg plugin.
set -euo pipefail
if [ "$(id -u)" -ne 0 ]; then exec sudo -E bash "$0" "$@"; fi
apt-get update -y
apt-get install -y --no-install-recommends \
  libqt6waylandclient6 libqt6waylandcompositor6 \
  libqt6svg6 libgl1-mesa-dri libegl-mesa0 \
  fonts-inter-variable fonts-roboto-unhinted fonts-noto-color-emoji fonts-noto-core \
  imagemagick || \
apt-get install -y --no-install-recommends \
  libqt6waylandclient6 libqt6waylandcompositor6 \
  libqt6svg6 libgl1-mesa-dri libegl-mesa0 \
  fonts-inter-variable fonts-roboto-unhinted fonts-noto-color-emoji \
  imagemagick
