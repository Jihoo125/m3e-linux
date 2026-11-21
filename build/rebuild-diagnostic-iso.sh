#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
WORK=/var/tmp/materialized-iso
python3 - <<'PY'
from pathlib import Path
p = Path('/var/tmp/materialized-iso/chroot/home/materialized/.profile')
s = p.read_text().replace('exec /usr/bin/materialized-session >>/tmp/materialized-session.log 2>&1 >/dev/ttyS0\n', 'exec /usr/bin/materialized-session >>/tmp/materialized-session.log 2>&1\n')
p.write_text(s)
PY
bash "$ROOT/platform/iso/finalize-iso.sh" > /var/tmp/materialized-finalize.log 2>&1
tail -15 /var/tmp/materialized-finalize.log
