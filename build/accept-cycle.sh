#!/usr/bin/env bash
# Clean 0.2.1 acceptance cycle after the focus-on-map fix.
set -eu
cd /mnt/c/Users/iris8/Downloads/yes
cmake --build build/matcomp -j4
cmake --build build/matterminal -j4
ctest --test-dir build/dev --output-on-failure | tail -3
bash build/rebuild-session-iso.sh > /var/tmp/rebuild-final.log 2>&1
tail -2 /var/tmp/rebuild-final.log
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode uefi --timeout 360
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.1-amd64.iso --mode bios --timeout 360
cat build/iso/materialized-0.2.1-amd64.iso.sha256