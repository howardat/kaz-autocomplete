# Remove KazType. Run from an elevated PowerShell.
#Requires -RunAsAdministrator
$ErrorActionPreference = 'Continue'

$Dest = Join-Path $env:ProgramW6432 'KazType'

try { & (Join-Path $PSScriptRoot 'languages.ps1') -Remove } catch { Write-Warning "Language list: $_" }

foreach ($t in @(@{ Arch = 'x64'; Dir = 'System32' }, @{ Arch = 'arm64'; Dir = 'System32' }, @{ Arch = 'x86'; Dir = 'SysWOW64' })) {
    $dll = Join-Path $Dest "$($t.Arch)\KazType.dll"
    if (Test-Path $dll) { & "$env:windir\$($t.Dir)\regsvr32.exe" /s /u $dll }
}
# Files still loaded by running apps cannot be deleted; they go on the next reboot.
Remove-Item $Dest -Recurse -Force -ErrorAction SilentlyContinue
if (Test-Path $Dest) { Write-Warning "Some files are in use. Delete $Dest after a restart." }

Write-Host 'Uninstalled. Your user dictionary is kept in %APPDATA%\KazType.'
