#!/usr/bin/env bash
set -euo pipefail
printf '== live-build helpers ==\n'
ls /usr/lib/live/build/lb_binary_grub*
printf '\n== EFI helper ==\n'
grep -n -E 'grub-mkimage|grub-mkstandalone|efi.img|grub-efi|xorriso|genisoimage' /usr/lib/live/build/lb_binary_grub-efi /usr/lib/live/build/lb_binary_iso | head -100 || true
printf '\n== Existing GRUB configuration ==\n'
cat /var/tmp/materialized-iso/chroot/binary/boot/grub/grub.cfg
printf '\n== Hook execution ==\n'
grep -n -E 'hooks/live|hooks.*chroot' /usr/lib/live/build/lb_chroot_hooks | head -50 || true
printf '\n== Live user ==\n'
grep materialized /var/tmp/materialized-iso/chroot/etc/passwd || true
ls -ld /var/tmp/materialized-iso/chroot/home/* 2>/dev/null || true
printf '\n== Missing linked libraries ==\n'
for binary in /usr/bin/matshell /usr/bin/matcomp /usr/share/materialized/qml/Materialized/libm3plugin.so; do
    LD_LIBRARY_PATH=/var/tmp/materialized-iso/chroot/usr/lib/x86_64-linux-gnu ldd "/var/tmp/materialized-iso/chroot$binary" | grep 'not found' || true
done
