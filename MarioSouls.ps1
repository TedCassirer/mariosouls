#Requires -Version 5.1
<#
.SYNOPSIS
Install or uninstall MarioSouls from a prepared source checkout.
.EXAMPLE
.\MarioSouls.ps1 install
.EXAMPLE
.\MarioSouls.ps1 uninstall
.EXAMPLE
.\MarioSouls.ps1 install -WhatIf
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('install', 'uninstall')]
    [string]$Action,
    [string]$GameDirectory = $env:MARIOSOULS_GAME_DIR,
    [string]$Rom,
    [string]$CollisionDirectory,
    [string]$EquipmentDirectory,
    [string]$Python = $env:MARIOSOULS_PYTHON,
    [Alias('WhatIf')][switch]$Preview
)

$ErrorActionPreference = 'Stop'
# Python is already required by the source build. Never download a runtime or
# change the machine's execution policy. The implementation uses the stdlib only.
$candidates = @()
if ($Python) {
    $candidates = @($Python)
} else {
    foreach ($name in @('python', 'python3', 'py')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command -and $command.Source -notlike '*\WindowsApps\*') {
            $candidates += $command.Source
        }
    }
    $candidates += Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
}
$interpreter = $null
foreach ($candidate in $candidates) {
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
    & $candidate -c 'import sys; sys.exit(0 if sys.version_info >= (3, 11) else 1)' 2>$null
    if ($LASTEXITCODE -eq 0) { $interpreter = $candidate; break }
}
if (-not $interpreter) {
    throw 'Python 3.11+ is required. Install Python or pass -Python C:\path\python.exe.'
}

$arguments = @((Join-Path $PSScriptRoot 'framework/hosts/ds1ptde/setup.py'), $Action)
foreach ($pair in @(
    @('--game', $GameDirectory), @('--rom', $Rom),
    @('--collision-directory', $CollisionDirectory), @('--equipment-directory', $EquipmentDirectory)
)) {
    if ($pair[1]) { $arguments += @($pair[0], $pair[1]) }
}
if ($Preview) { $arguments += '--dry-run' }
& $interpreter @arguments
if ($LASTEXITCODE -ne 0) { throw "MarioSouls $Action failed; see the message above." }
