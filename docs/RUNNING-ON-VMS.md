# Running Materialized Linux on VM hypervisors

**Applies to:** `materialized-0.2.0-amd64.iso` (Alpha 0.2.0) and later.
**Status key:** ✅ verified by the project's acceptance harness · 🧪 expected to
work from driver/architecture analysis, not yet exercised by the harness.

The ISO is a Debian 13 (trixie) x86_64 live image with a hybrid GRUB 2 boot
record (UEFI **and** legacy BIOS). The desktop is a real wlroots compositor
(`matcomp`) rendering with the **pixman software renderer** onto a DRM/KMS
output. That single fact determines most of the guidance below:

| Requirement | Why |
|---|---|
| x86_64 guest | Kernel and userspace are amd64 only. |
| A virtual GPU with a **DRM/KMS kernel driver** | `matcomp` uses the wlroots DRM backend. virtio-gpu (`virtio_gpu`), VMware/VBox SVGA (`vmwgfx`) and Hyper-V (`hyperv_drm`, kernel ≥ 5.14) qualify. Pure framebuffer devices (e.g. Hyper-V Gen 1 `hyperv_fb`, VBoxVGA, `-device VGA` in some modes) do **not**. |
| **Secure Boot OFF** | The GRUB images in the ISO are unsigned. |
| ≥ 2 vCPUs, ≥ 3 GB RAM | The acceptance harness uses `-smp 2 -m 3072`; Firefox ESR plus the QML shell need the headroom. 2 GB boots but is tight. |
| 3D acceleration **not** required | Rendering is software (pixman + Qt `QT_QUICK_BACKEND=software`). Leave 3D off; it only adds driver variance. |
| UEFI or BIOS firmware | Both boot paths are tested. UEFI needs OVMF/EDK2 or the hypervisor's EFI. |

No installation is performed: everything runs live from the ISO. The session
auto-logins on tty1 as `materialized` / `materialized` (evaluation credential —
VM use only).

---

## 1. QEMU / KVM — ✅ verified (UEFI and BIOS)

This is exactly what `tests/vm/run-qemu.sh` runs in CI.

### UEFI (OVMF)

```bash
# One-time: private copy of the UEFI variable store
cp /usr/share/OVMF/OVMF_VARS_4M.fd ./OVMF_VARS.fd

qemu-system-x86_64 \
  -machine q35,accel=kvm -cpu host -m 3072 -smp 2 \
  -cdrom materialized-0.2.0-amd64.iso -boot d \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file=./OVMF_VARS.fd \
  -device virtio-vga -device virtio-rng-pci \
  -netdev user,id=n0 -device e1000e,netdev=n0
```

### Legacy BIOS

Same command **without** the two `-drive if=pflash…` lines (SeaBIOS is the
default firmware). The BIOS boot path is verified too.

### Without KVM (TCG)

Replace `-machine q35,accel=kvm -cpu host` with `-machine q35,accel=tcg -cpu max`.
It works but is ~10× slower; give the session 3–5 minutes to reach the desktop.
The harness default timeout is 300 s for this reason.

### Display / capture notes

- Use **`-device virtio-vga`** (verified). Plain `-device VGA` has produced
  "Guest has not initialized the display" captures with this compositor; avoid it.
- Headless acceptance: `-display none -serial file:boot.log` plus a QEMU monitor
  socket for `screendump` (see `tests/vm/run-qemu.sh`). The shell prints
  `Materialized session started` on the serial console only after the QML window
  is genuinely exposed.

### Windows hosts

- **Recommended:** run the harness inside WSL2 (`wsl -d Ubuntu -u root -- bash
  tests/vm/run-qemu.sh --iso … --mode uefi`). For KVM speed enable nested
  virtualization in `%USERPROFILE%\.wslconfig`:
  ```ini
  [wsl2]
  nestedVirtualization=true
  ```
  then `wsl --shutdown`. Without it QEMU falls back to TCG (slow but fine).
- Native QEMU for Windows: use `-accel whpx` (Windows Hypervisor Platform) with
  `-cpu qemu64` and the same device set; OVMF files ship with many QEMU builds
  (`share/edk2-x86_64-code.fd`). 🧪 Not harness-verified.

---

## 2. VirtualBox — 🧪 expected to work (VMSVGA)

VirtualBox's **VMSVGA** controller exposes a `vmwgfx` DRM device, which the
wlroots DRM backend can drive with pixman. Do **not** use VBoxVGA (framebuffer
only) or enable 3D acceleration.

GUI settings:

| Setting | Value |
|---|---|
| Type / Version | Linux / Debian (64-bit) |
| Base memory | 3072 MB |
| Processors | 2 |
| System → Motherboard | Enable I/O APIC; **EFI: optional** (both firmware paths boot); Secure Boot off if present |
| Display → Graphics Controller | **VMSVGA** |
| Display → Video Memory | 128 MB |
| Display → 3D Acceleration | **unchecked** |
| Storage | Attach the ISO to the IDE/optical drive, boot order optical first |
| Network | NAT (Intel PRO/1000 or virtio-net both work with NetworkManager) |

Equivalent CLI:

```bash
VBoxManage createvm --name materialized --ostype Debian_64 --register
VBoxManage modifyvm materialized --memory 3072 --cpus 2 --ioapic on \
  --graphicscontroller vmsvga --vram 128 --firmware efi   # or: --firmware bios
VBoxManage storagectl materialized --name IDE --add ide
VBoxManage storageattach materialized --storagectl IDE --port 0 --device 0 \
  --type dvddrive --medium materialized-0.2.0-amd64.iso
VBoxManage startvm materialized
```

If the desktop never appears, check the guest console (Ctrl+Alt+F2) for
`matcomp` errors: a missing DRM node means the wrong graphics controller.

---

## 3. VMware Workstation / Player / Fusion (Intel hosts) — 🧪 expected to work

VMware's SVGA II device is driven by `vmwgfx` in the guest — a real DRM driver.
Create a "Linux / Debian 12.x 64-bit" (or Other Linux 6.x 64-bit) VM, or use this
`.vmx` baseline:

```ini
guestOS = "debian12-64"
firmware = "efi"                  # or "bios"; both boot paths work
uefi.secureBoot.enabled = "FALSE"
memsize = "3072"
numvcpus = "2"
svga.present = "TRUE"
svga.vramSize = "134217728"       # 128 MB
mks.enable3d = "FALSE"            # software rendering; 3D not needed
ethernet0.virtualDev = "e1000e"
cdrom0.imageType = "cdrom-image"
cdrom0.fileName = "materialized-0.2.0-amd64.iso"
cdrom0.startConnected = "TRUE"
```

Notes:

- Keep **Accelerate 3D graphics off** (Workstation: VM Settings → Display).
- VMware Tools / open-vm-tools are not required for the desktop.
- VMware on **Apple Silicon cannot run x86_64 guests** — use UTM (§6) there.

---

## 4. Hyper-V (Windows) — 🧪 expected to work, Generation 2 only

Debian 13's kernel (6.12) includes `hyperv_drm`, a DRM driver for the Hyper-V
synthetic video device, so the compositor has a KMS output to render to.
Generation 1 exposes only `hyperv_fb` (framebuffer) and will **not** run the
desktop.

```powershell
New-VM -Name materialized -Generation 2 -MemoryStartupBytes 3GB `
  -NewVHDPath "$env:PUBLIC\Documents\Hyper-V\materialized.vhdx" -NewVHDSizeBytes 8GB `
  -SwitchName "Default Switch"
Set-VMProcessor -VMName materialized -Count 2
Set-VMFirmware  -VMName materialized -EnableSecureBoot Off      # unsigned GRUB
Add-VMDvdDrive  -VMName materialized -Path .\materialized-0.2.0-amd64.iso
$dvd = Get-VMDvdDrive -VMName materialized
Set-VMFirmware  -VMName materialized -FirstBootDevice $dvd
Start-VM        -VMName materialized
vmconnect       localhost materialized
```

Caveats:

- **Secure Boot must be Off** (the ISO's GRUB is unsigned); the "Microsoft UEFI
  Certificate Authority" template will also reject it.
- Enhanced Session Mode is irrelevant (it is an RDP channel); use the basic
  console window.
- If `hyperv_drm` fails to bind on your build, the fallback is QEMU/WSL2 (§1),
  which is the verified path on Windows.

---

## 5. GNOME Boxes / virt-manager (libvirt) — 🧪 expected to work

Boxes: create a VM from the ISO, choose ≥ 3 GB RAM; Boxes defaults to virtio
video + UEFI, which matches the requirements. If it offers a "3D acceleration"
toggle, leave it off.

virt-manager / raw libvirt: firmware **UEFI (OVMF)**, and:

```xml
<os firmware='efi'>
  <type arch='x86_64' machine='q35'>hvm</type>
  <boot dev='cdrom'/>
</os>
<features><acpi/><apic/></features>
<cpu mode='host-passthrough'/>
<memory unit='GiB'>3</memory>
<vcpu>2</vcpu>
<devices>
  <video><model type='virtio' heads='1' primary='yes'/></video>
  <graphics type='spice' autoport='yes'/>
  <rng model='virtio'><backend model='random'>/dev/urandom</backend></rng>
</devices>
```

`<model type='virtio'/>` is the same `virtio_gpu` device the verified QEMU run
uses. Do not select QXL/VGA-only models if you want the desktop (they lack the
DRM path this compositor needs).

---

## 6. UTM on Apple Silicon (macOS) — 🧪 works by emulation, slow

UTM can **emulate** x86_64 with QEMU TCG:

1. Create a new VM → **Emulate** → Linux.
2. Architecture **x86_64**, machine `q35`, ≥ 3072 MB RAM, 2 CPUs.
3. Boot ISO: the Materialized ISO.
4. Display: **virtio-gpu-pci** (DRM `virtio_gpu`); leave "GPU acceleration"
   (virgl/ANGLE) off — rendering is software anyway.
5. UEFI boot is UTM's default for x86_64 emulation; keep Secure Boot off.

Expect a very slow boot (TCG on ARM translating x86): several minutes to the
desktop, and sluggish Firefox. This is inherent to emulation, not a bug in the
image. Parallels Desktop on Apple Silicon runs ARM guests only and therefore
**cannot** run this ISO.

---

## 7. Inside the VM (all hypervisors)

- **Autologin:** tty1 starts the session automatically; no login needed.
- **Credentials (recovery TTYs):** `materialized` / `materialized`.
- **Hotkeys:** `Ctrl+Alt+T` terminal, `Ctrl+Alt+B` Firefox ESR (Material-themed
  chrome). XDG autostart also launches both at session start.
- **Recovery:** `Ctrl+Alt+F2` … `F6` give login TTYs; the GRUB "safe graphics"
  entry (`nomodeset`) is a *recovery shell only* — with `nomodeset` there is no
  DRM output, so the desktop intentionally does not start.
- **Serial diagnostics:** with a serial console attached (`console=ttyS0`), the
  kernel, compositor and an in-guest watchdog report state; the shell prints
  `Materialized session started` once the QML window is exposed.
- **Network:** DHCP via NetworkManager on NAT/user-mode networking; Firefox
  works out of the box.

## 8. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| GRUB refuses to load / "Security violation" | Secure Boot enabled | Disable Secure Boot (all hypervisors). |
| Boots to login prompt, desktop never appears | Virtual GPU without a DRM driver (VBoxVGA, Hyper-V Gen 1, plain fbdev) | Switch to virtio-vga / VMSVGA / VMware SVGA / Hyper-V Gen 2. |
| QEMU capture: "Guest has not initialized the display" | `-device VGA` instead of virtio-vga | Use `-device virtio-vga`. |
| Extremely slow boot / UI | TCG emulation (no KVM/WHPX/nested virt) | Enable KVM, WHPX, or WSL2 `nestedVirtualization=true`; or accept slowness on UTM. |
| Black screen but serial shows `Materialized session started` | Capture taken before first modeset, or wrong display device | Wait a few seconds; re-capture; verify video model. |
| Firefox exits immediately in a restrictive sandbox | User namespaces blocked by the hypervisor/guest policy | The live `matbrowser` wrapper already relaxes the content sandbox for evaluation; check `/tmp/matbrowser.log` in a recovery TTY. |
| Only 1024×768 resolution | Compositor picks the connector's preferred mode | Expected for the alpha; mode selection is on the roadmap. |

## 9. Verification matrix

| Hypervisor / config | Firmware | Graphics | Status |
|---|---|---|---|
| QEMU q35 + KVM | UEFI (OVMF 4M) | virtio-vga | ✅ harness-passed 2026-11-21 |
| QEMU q35 + KVM | BIOS (SeaBIOS) | virtio-vga | ✅ harness-passed 2026-11-21 |
| QEMU q35 + TCG | UEFI / BIOS | virtio-vga | ✅ same image, slow path |
| VirtualBox 7.x | EFI or BIOS | VMSVGA, 3D off | 🧪 untested |
| VMware Workstation/Player/Fusion (Intel) | EFI or BIOS | SVGA II, 3D off | 🧪 untested |
| Hyper-V Gen 2, Secure Boot off | UEFI | hyperv_drm | 🧪 untested |
| GNOME Boxes / virt-manager | UEFI | virtio video | 🧪 untested |
| UTM (Apple Silicon, emulate x86_64) | UEFI | virtio-gpu-pci | 🧪 untested, slow |
| Any config with Secure Boot on, or fbdev-only video | — | — | ❌ unsupported |

Reproduce the verified rows with:

```bash
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.0-amd64.iso --mode uefi
bash tests/vm/run-qemu.sh --iso build/iso/materialized-0.2.0-amd64.iso --mode bios
```
