#!/usr/bin/env bash
set -euo pipefail
for f in /mnt/c/Users/iris8/Downloads/yes/build/vm/browserdbg.log /mnt/c/Users/iris8/Downloads/yes/build/vm/boot-uefi.log; do
    printf '== %s ==\n' "$f"
    grep -a -nE 'segfault|general protection|BUG|trap|Oops|matcomp\[' "$f" | tail -8 || true
    printf -- '-- last 6 lines --\n'
    tail -6 "$f"
done
