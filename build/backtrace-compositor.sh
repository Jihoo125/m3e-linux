#!/usr/bin/env bash
set -euo pipefail
if ! command -v gdb >/dev/null 2>&1; then
    apt-get update -y >/dev/null && apt-get install -y --no-install-recommends gdb >/dev/null
fi
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
export XDG_RUNTIME_DIR=/tmp WLR_BACKENDS=headless WLR_RENDERER=pixman
ulimit -c 0
exec gdb -batch -ex 'set pagination off' -ex run -ex bt --args ./build/matcomp/matcomp --headless --self-test
