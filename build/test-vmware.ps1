# VMware Workstation acceptance for the Materialized live ISO.
# Boots an isolated test VM (never the user's own VM), proves keyboard/pointer
# input through VMware's VNC remote display, gates on the guest serial log,
# captures the console, and stops the VM.
# Usage: powershell -ExecutionPolicy Bypass -File build\test-vmware.ps1
$ErrorActionPreference = 'Stop'
$vmrun  = 'C:\Program Files\VMware\VMware Workstation\vmrun.exe'
$vmx    = 'D:\Virtual Machines\materialized-021\materialized-021.vmx'
$serial = 'D:\Virtual Machines\materialized-021\serial.log'
$out    = 'C:\Users\iris8\Downloads\yes\build\vm'
$python = 'C:\Users\iris8\AppData\Local\Programs\Python\Python315\python.exe'
$openssl = 'C:\Program Files\Git\usr\bin\openssl.exe'

if (Test-Path $serial) { Remove-Item $serial -Force }
& $vmrun -T ws start $vmx nogui
if ($LASTEXITCODE -ne 0) { throw "vmrun start failed ($LASTEXITCODE)" }

$marker = $false
for ($i = 0; $i -lt 300; $i++) {
    Start-Sleep -Seconds 1
    if ((Test-Path $serial) -and (Select-String -Path $serial -Pattern 'Materialized session started' -Quiet -Encoding ascii)) {
        $marker = $true; break
    }
}
if (-not $marker) { & $vmrun -T ws stop $vmx hard; throw 'session marker never appeared on VMware serial' }
Write-Host 'VMware: live session marker found on serial.'
Start-Sleep -Seconds 3

& $python 'C:\Users\iris8\Downloads\yes\build\vnc-input-test.py' 127.0.0.1 5901 matvm123 $openssl
if ($LASTEXITCODE -ne 0) { & $vmrun -T ws stop $vmx hard; throw "VNC input test failed ($LASTEXITCODE)" }

$typed = $false
for ($i = 0; $i -lt 30; $i++) {
    Start-Sleep -Seconds 1
    if (Select-String -Path $serial -Pattern '^input-ok' -Quiet -Encoding ascii) { $typed = $true; break }
}
& $vmrun -T ws captureScreen $vmx (Join-Path $out 'vmware-desktop.png')
Write-Host "VMware: console captured to $out\vmware-desktop.png"
& $vmrun -T ws stop $vmx hard
if (-not $typed) { throw 'VMware: typed command did not reach the guest shell' }
Write-Host 'VMware acceptance passed: boot + keyboard + pointer verified.'