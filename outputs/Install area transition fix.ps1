param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Install cannon and Mario loading fixes.ps1') -CheckOnly:$CheckOnly
