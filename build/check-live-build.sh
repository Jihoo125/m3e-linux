#!/usr/bin/env bash
set -euo pipefail
lb config --help | grep -E 'firmware|updates|security' | head -20 || true
