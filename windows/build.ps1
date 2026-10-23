# Build KazType for every Windows architecture and assemble dist\.
# Needs Visual Studio 2022 (Desktop C++, plus the ARM64 tools for arm64) and CMake.
#   .\build.ps1              build, run core tests, write dist\
#   .\build.ps1 -Arch x64    one architecture only
# With Inno Setup 6.3+ installed and all architectures built, it also writes
# dist\KazType-Setup-<version>.exe.
param([string[]]$Arch = @('x64', 'x86', 'arm64'))
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

if (-not (Test-Path Resources\kaztype.bin)) { throw 'Missing model. Run: uv run python -m ml.export_win' }
$platform = @{ x64 = 'x64'; x86 = 'Win32'; arm64 = 'ARM64' }

Remove-Item dist -Recurse -Force -ErrorAction SilentlyContinue
foreach ($a in $Arch) {
    cmake -S . -B "build\$a" -A $platform[$a] | Out-Host
    if ($LASTEXITCODE) { throw "cmake configure failed for $a" }
    cmake --build "build\$a" --config Release | Out-Host
    if ($LASTEXITCODE) { throw "build failed for $a" }
    # Tests run on the host; skip architectures it cannot execute.
    $native = $env:PROCESSOR_ARCHITECTURE -eq 'ARM64' -or $a -ne 'arm64'
    if ($native) {
        ctest --test-dir "build\$a" -C Release --output-on-failure | Out-Host
        if ($LASTEXITCODE) { throw "tests failed for $a" }
    }
    New-Item -ItemType Directory "dist\$a" -Force | Out-Null
    Copy-Item "build\$a\Release\KazType.dll", Resources\kaztype.bin "dist\$a\"
}
Copy-Item install.ps1, uninstall.ps1, languages.ps1 dist\
Write-Host "Built: $((Resolve-Path dist).Path). Install with: dist\install.ps1 (as administrator)"

# The installer bundles every architecture.
$iscc = Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
if (-not $iscc) { $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" }
if (@('x64', 'x86', 'arm64' | Where-Object { $Arch -notcontains $_ }).Count) {
    Write-Host 'Skipping installer: needs all of x64, x86, arm64.'
} elseif (-not (Test-Path $iscc)) {
    Write-Warning 'Skipping installer: Inno Setup 6 not found (winget install JRSoftware.InnoSetup).'
} else {
    $version = [regex]::Match((Get-Content tsf\KazType.rc -Raw), '"ProductVersion", "([^"]+)"').Groups[1].Value
    & $iscc /Q "/DAppVersion=$version" installer\KazType.iss | Out-Host
    if ($LASTEXITCODE) { throw 'installer build failed' }
    Write-Host "Installer: $((Resolve-Path "dist\KazType-Setup-$version.exe").Path)"
}
