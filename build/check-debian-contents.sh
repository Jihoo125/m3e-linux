#!/usr/bin/env bash
set -euo pipefail
for path in dists/trixie/Contents-amd64.gz dists/trixie/main/Contents-amd64.gz dists/trixie/main/Contents-source.gz dists/trixie/Contents-source.gz; do
    printf '%s -> ' "$path"
    curl -s -o /dev/null -w '%{http_code}\n' "http://deb.debian.org/debian/$path"
done
