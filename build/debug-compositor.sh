#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
command -v gdb || true
sed -n '230,260p' /usr/include/wlr/types/wlr_scene.h
export XDG_RUNTIME_DIR=/tmp WLR_BACKENDS=headless WLR_RENDERER=pixman
ulimit -c 0
if command -v gdb >/dev/null 2>&1; then
    gdb -batch -ex run -ex bt --args ./build/matcomp/matcomp --headless --self-test
fi
