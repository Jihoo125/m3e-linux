#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/iris8/Downloads/yes
apt-get install -y --no-install-recommends npm node-typescript >/dev/null
mkdir -p build/reference
cd build/reference
if [ ! -f node_modules/typescript/package.json ]; then npm install --no-audit --no-fund typescript@4.9.5; fi
