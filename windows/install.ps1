# Install KazType for all apps and add it to your input methods.
# Run from an elevated PowerShell:  powershell -ExecutionPolicy Bypass -File install.ps1
#Requires -RunAsAdministrator
$ErrorActionPreference = 'Stop'

$Dest = Join-Path $env:ProgramW6432 'KazType'
$native = if ($env:PROCESSOR_ARCHITEW6432) { $env:PROCESSOR_ARCHITEW6432 } else { $env:PROCESSOR_ARCHITECTURE }
$nativeArch = if ($native -eq 'ARM64') { 'arm64' } else { 'x64' }

# Each architecture's DLL serves apps of that architecture; 32-bit apps need x86.
# x64 apps emulated on ARM64 are not supported.
$targets = @(
    @{ Arch = $nativeArch; Regsvr = "$env:windir\System32\regsvr32.exe" },
    @{ Arch = 'x86'; Regsvr = "$env:windir\SysWOW64\regsvr32.exe" }
)

foreach ($t in $targets) {
    $src = Join-Path $PSScriptRoot $t.Arch
    if (-not (Test-Path "$src\KazType.dll")) { Write-Warning "No $($t.Arch) build in $src, skipping"; continue }
    $dir = Join-Path $Dest $t.Arch
    New-Item -ItemType Directory $dir -Force | Out-Null
    foreach ($f in 'KazType.dll', 'kaztype.bin') {
        $target = Join-Path $dir $f
        # Apps that use KazType keep the old DLL loaded; moving it aside still works.
        if (Test-Path $target) {
            $old = "$target.$([guid]::NewGuid().ToString('N')).old"
            Move-Item $target $old
            Remove-Item $old -ErrorAction SilentlyContinue
        }
        Copy-Item (Join-Path $src $f) $target
    }
    & $t.Regsvr /s (Join-Path $dir 'KazType.dll')
    if ($LASTEXITCODE) { throw "regsvr32 failed for $($t.Arch) ($LASTEXITCODE)" }
    Write-Host "Registered $($t.Arch): $dir"
}

& (Join-Path $PSScriptRoot 'languages.ps1')

Write-Host 'Installed. Switch to "Kazakh - KazType" with Win+Space.'
Write-Host 'Apps that were already open pick it up after a restart.'
