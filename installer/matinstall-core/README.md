# matinstall-core

`matinstall-core` is a dependency-free C++20 installation engine foundation for
Materialized Linux. It builds a static `matinstall_core` library and a
`matinstall-core` command-line program. It does **not** require Qt, a JSON
library, or a graphical session.

> **Warning:** `run` replaces the GPT label and every partition on one whole
> disk. `plan` and `validate` never perform a destructive operation. There is no
> default install on startup, no timer, no background watcher, and no command
> executed merely by loading this library.

## Build and test

On Linux (including WSL):

```sh
cmake -S installer/matinstall-core -B /tmp/matinstall-core-build \
  -DMATINSTALL_BUILD_TESTS=ON -DMATINSTALL_WERROR=ON
cmake --build /tmp/matinstall-core-build
ctest --test-dir /tmp/matinstall-core-build --output-on-failure
```

The CMake file is standalone and requires CMake 3.22 or newer. Tests use plain
C++ assertions (no Catch2 download, Qt, or third-party dependency). They use
fake disk sizes and fake command runners for all planner/deployment cases. The
only real subprocess tests use local `printf`, `cat`, and `true`-like commands;
they never invoke partitioning, mounting, formatting, chroot, or GRUB.

## CLI

```text
matinstall-core probe
matinstall-core plan ANSWER.json
matinstall-core validate ANSWER.json
matinstall-core run ANSWER.json --i-understand-this-erases-data
```

* `probe` runs read-only `lsblk --json --bytes --paths --tree` and validates the
  returned JSON. It prints disk, partition, filesystem and mount information.
* `validate` strictly parses all required fields, rejects unsafe names and
  paths, and checks geometry only when `disk_size_bytes` is supplied. Without a
  size it reports `disk_size_validated:false`; it does not touch real disks.
* `plan` prints a JSON dry-run partition/deploy plan, including sector geometry,
  the exact `sfdisk` stdin script, shell-escaped audit strings for every command,
  destructive-step flags, warnings and mount registrations.
  `disk_probed` identifies whether geometry came from the answer or a live
  read-only probe. With an answer-supplied size no disk is read. It rejects a
  live target that is read-only, mounted, or has holders.
* `run` performs the actual install and returns a nonzero status on error. It
  requires **both** `destructive:true` in the answer and the exact
  `--i-understand-this-erases-data` flag. Omitting either is a safe refusal.
  `plan` is always a dry run even when `destructive:true` is present.
* `run` additionally requires effective uid 0, an exact whole-disk `lsblk`
  entry, a direct existing block device, no read-only flag, no mounted
  descendants or device holders, a regular readable valid SquashFS, available
  host tools, and a free `/mnt/materialized` mountpoint.

For example, inspect without changing anything:

```sh
./matinstall-core validate installer/matinstall-core/examples/answer.json
./matinstall-core plan installer/matinstall-core/examples/answer.json
./matinstall-core probe
```

The example has a **fictional 120 GiB** size so `plan` is a dry-run and does not
need `/dev/sda` to exist. Change it for a real VM and remove the size to probe
actual geometry.

## Answer format

See [`examples/answer.json`](examples/answer.json):

```json
{
  "disk": "/dev/sda",
  "disk_size_bytes": 128849018880,
  "firmware": "uefi",
  "filesystem": {
    "root": "ext4",
    "separate_home": false
  },
  "swapfile_mib": 4096,
  "source": "/cdrom/filesystem.squashfs",
  "hostname": "materialized",
  "locale": "en_US.UTF-8",
  "timezone": "Europe/London",
  "keyboard": "us",
  "user": {
    "name": "iris",
    "full_name": "Iris Example",
    "password": "replace-this-in-your-private-answer"
  },
  "destructive": false
}
```

| Field | Required | Meaning |
|---|---|---|
| `disk` | yes | Direct whole-disk `/dev/...` block path. `/dev/mapper`, loop, RAM, zram, optical, traversal and ambiguous paths are rejected. |
| `disk_size_bytes` | no | Exact byte geometry for an offline plan/test. If omitted by `plan`, it is obtained with read-only `lsblk`. |
| `logical_sector_size` | no | 512/1024/2048/4096; normally 512. Required to match the probed sector size if provided. |
| `firmware` | no | `uefi` (default) or `bios`. No auto-detection is assumed by unattended answers. |
| `filesystem.root` | no | `ext4` (default) or `btrfs`. |
| `filesystem.separate_home` | no | Creates a separate home partition for ext4. Btrfs already has an `@home` subvolume and rejects this combination explicitly. |
| `root_size_mib` | with ext4 separate home | Root size, at least 8192 MiB. Defaults to half the available space. Home uses the remaining space. |
| `home_size_mib` | no | Reserved but rejected: remaining-space-only home is the supported v1 policy. |
| `swapfile_mib` | no | 0 (default) disables swap. Maximum 1 TiB; root must retain 8192 MiB. Creates `/swapfile` on ext4, or a NOCOW `/swap/swapfile` in Btrfs `@swap`. |
| `source` | yes | Absolute SquashFS path. `run` requires a regular readable image, not a symlink or device, and checks `unsquashfs -s`. |
| `hostname` | yes | RFC 1123-style hostname. |
| `locale` | no | Simple locale name such as `en_US.UTF-8`; default `en_US.UTF-8`. It must already be defined by the target image's locale data. |
| `timezone` | no | Zoneinfo path relative to `/usr/share/zoneinfo`; default `UTC`; target existence is checked. |
| `keyboard` | no | Debian XKB layout name, default `us`. |
| `user.name` | yes | 1–32-character lowercase Linux user name. Reserved system accounts are rejected. |
| `user.full_name` | no | passwd/gecos-compatible display name; defaults to user name. |
| `user.password` | yes | 8–1024 bytes, no control characters. Passed to `chpasswd` through stdin only. |
| `destructive` | no | Default false. `run` refuses unless explicitly true; `plan` ignores it for execution purposes. |

Unknown keys are currently ignored for forward compatibility; duplicate JSON
keys, non-integral sizes, invalid UTF-8, control characters and wrong field
types are rejected. The 1 MiB answer-file cap limits accidental resource
exhaustion. Keep answer files containing passwords readable only by the
intended user/root and delete them after use. Passwords are never emitted by
`plan`, step logs, or CLI success output. Other logs (squashfs/tool stderr)
should still be treated as private diagnostic data.

### Planned disk layouts

All sizes use sector arithmetic, avoid floating point, reserve 33 trailing GPT
sectors, and align ordinary data partitions to 1 MiB (2048 sectors on a
512-byte disk, 256 on a 4K disk).

| Firmware | Partitions |
|---|---|
| UEFI | 512 MiB FAT32 EFI System Partition at `/boot/efi`; root; optional ext4 `/home` |
| BIOS | 1 MiB `bios_grub` partition (`21686148-...`) after the MBR gap; root; optional ext4 `/home` |

Root defaults to ext4. Btrfs creates `@`, `@home`, `@var_log`, read-only
`@snapshots`, and optional `@swap`, with mountpoints created inside `@` before
mounting children. There is no swap partition. Swap is a file, initialized and
persisted in fstab with `nofail`. Btrfs swap allocation is explicitly written
and NOCOW; `fallocate` is not used. Hibernation, encryption, RAID/LVM,
device-mapper targets, custom partition editing and a Btrfs separate-home
partition are not implemented.

## What `run` does

1. Rechecks confirmations, root, source, disk identity, geometry, in-use state,
   mountpoint and tools before creating the partition table.
2. Executes `sfdisk` with the planned sector script, `partprobe` and `udevadm
   settle`, then checks that each partition device exists.
3. Runs real `mkfs.vfat`, `mkfs.ext4`, or `mkfs.btrfs` commands.
4. Mounts the target at `/mnt/materialized`, creates Btrfs subvolumes when
   requested, and uses real `unsquashfs -f -d` deployment.
5. Binds `/dev` and `/sys`, marks bind propagation slave, and mounts a fresh
   `/proc` before chroot configuration. `/run` is bound before GRUB generation.
6. Generates UUID-based fstab entries using a validated `blkid` result, and
   configures hostname, timezone, locale, keyboard, user, sudo group and locked
   root login. `chpasswd` receives the password via stdin; argv and logs are
   redacted. Sensitive-command stdout/stderr is suppressed even on failure.
7. Installs GRUB with `grub-install --target=x86_64-efi` or `--target=i386-pc`,
   updates initramfs and runs `update-grub`. The SquashFS must include these
   target tools, an appropriate kernel/initramfs, locale data, zoneinfo and
   `sudo` support.
8. Syncs and recursively unmounts all still-registered target mounts in reverse
   order. This cleanup occurs after success and failure. Busy mounts fall back
   to logged lazy recursive unmount; failure is propagated and never silently
   reported as successful cleanup.

Execution stops at the first failed command. The step runner captures bounded
stdout/stderr concurrently (while feeding stdin), logs command and step names,
and includes exit status/signal/tool diagnostics in errors. Commands are
executed with `fork`/`execve` after resolution below a fixed trusted `PATH`,
using explicit argv and only `PATH`/`LC_ALL=C` in the child environment,
not by interpolating arguments into a shell. Fixed file-oriented chroot scripts
use `sh -c`; every interpolated answer/device/UUID value is shell-escaped or
grammar-validated. `shell_escape()` is for auditable command text, not a license
to execute untrusted shell input.

Logs default to `/var/log/materialized/install-<unix-time>-<pid>.log`, are
created exclusively with no symlink following and mode 0600. Disk contents cannot be rolled back after partitioning: rollback means
**unmount and report**, not data restoration. A reboot is not performed by the
engine. Power loss between steps still needs operator/recovery handling.

## Polkit / system-daemon integration boundary

The production QML frontend must **not** invoke `pkexec matinstall-core run`
with an interactive command-line confirmation substituted by a shell. Instead:

1. A small system-bus daemon owns the root process and exposes the existing
   `org.materialized.Installer1` `Probe`, `Plan`, `Run`, `Cancel`, `Progress`,
   `Log` and `Finished` contract. It should be D-Bus activated with no ambient
   authority in the GUI process.
2. `Probe` and `Plan` can be implemented directly around the library. `Run`
   must check the caller's polkit `org.materialized.install` authorization
   using `auth_admin_keep`, validate the answer server-side, and pass both
   explicit confirmations through a narrowly typed request. Polkit is an
   additional authorization boundary; it does **not** replace
   `destructive:true` and `--i-understand-this-erases-data` on the CLI.
3. The daemon must serialize installs, prevent concurrent `Run` calls, retain a
   per-job id, stream redacted logs/progress, support cooperative cancellation
   between safe steps, re-probe the disk immediately before `sfdisk`, and
   unmount on every exit path. The root daemon should accept answers from a
   protected fd/Unix socket rather than a world-readable answer file.
4. A `.policy` action `org.materialized.install` should default to
   `auth_admin_keep` for active sessions and deny inactive sessions. A local
   rule may grant the live installer group this action, still with explicit
   authentication appropriate to the live environment. Do not use `yes` as a
   blanket polkit result. Ship the daemon and policy as separate integration
   artifacts after review; this component does not install either globally.

The present static library/CLI provides the parsing, planning, process and
step-machine foundation. The actual D-Bus service, policy/rule packaging,
frontend wiring, cancellation and daemon-side progress protocol are remaining
integration work.

## Remaining integration / limitations

* Real-hardware/VM end-to-end installs have not been executed by these unit
  tests. Test both UEFI and BIOS boot after installation, mount/fstab behavior,
  GRUB, sudo, locales and first-login behavior with a disposable QEMU disk.
* UEFI installs require `/sys/firmware/efi/efivars` and verify `efivarfs` before
  partitioning; an unregistered boot entry is not silently accepted.
* Target-tool preflight occurs after deployment (destructive partitioning has
  already happened). Add SquashFS manifest verification before partitioning.
* `SystemProcessRunner` resolves executables below a fixed trusted path and uses
  `execve` with a minimal environment. Ordinary commands do not use a shell;
  explicit target scripts are reviewed below as remaining integration surface.
* There is no timeout/cancellation yet for a wedged privileged subprocess.
  Cleanup is best effort and lazy unmount is only a last resort.
* Btrfs snapshots are mounted read-only but snapshot management, recovery,
  rollback of formatted disks and atomic system upgrades are future work.
* Existing dual-boot detection/os-prober policy, disk selection UI, manual
  partition mode, Windows fast-start checks, Secure Boot support and hardware
  firmware auto-detection remain integration tasks.
