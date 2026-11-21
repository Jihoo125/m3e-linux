#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/iris8/Downloads/yes
printf '== unified build ==\n'
cmake --build build/dev -j4 > /var/tmp/devbuild.log 2>&1
printf '== installer ctest ==\n'
ctest --test-dir build/dev --output-on-failure
printf '== compositor self-test ==\n'
bash build/test-compositor.sh
printf '== wayland client ==\n'
bash build/test-wayland-client.sh
printf '== installer suite ==\n'
bash build/test-installer.sh > /var/tmp/installer.log 2>&1
tail -5 /var/tmp/installer.log
printf '== token compiler ==\n'
python3 -m unittest tests.unit.token_compiler_test -q
printf '== core apps ==\n'
bash build/build-core-apps.sh > /var/tmp/apps.log 2>&1
grep -E 'Test #|tests passed' /var/tmp/apps.log || true
printf '== final ISO ==\n'
cat build/iso/materialized-0.2.1-amd64.iso.sha256
sha256sum -c build/iso/materialized-0.2.1-amd64.iso.sha256
