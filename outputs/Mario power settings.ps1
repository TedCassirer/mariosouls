param(
    [ValidateRange(0,15)][int]$UpgradeLevel = 0,
    [ValidateRange(0,1200)][double]$FixedPower = 0,
    [switch]$DisableStatScaling,
    [switch]$DisableEquipmentPenalty
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$taskPython = 'C:\Users\Ted\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$taskScaling = if ($DisableStatScaling) { 0 } else { 1 }
$taskWeight = if ($DisableEquipmentPenalty) { '0' } else { '0.01' }
$taskPower = $FixedPower.ToString([Globalization.CultureInfo]::InvariantCulture)
& $taskPython (Join-Path $taskRoot 'work\progression\settings.py') "UpgradeLevel=$UpgradeLevel" "FixedPower=$taskPower" "StatScaling=$taskScaling" "EquipmentCostPerWeight=$taskWeight"
if ($LASTEXITCODE -ne 0) { throw 'Mario settings could not be updated.' }
Write-Host 'Mario power settings updated. They apply within two seconds.'
Write-Host 'This development setting applies to Mario mode globally; no titanite is spent and no save inventory is changed.'
