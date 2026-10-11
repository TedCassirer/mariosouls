param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$taskRepo = Split-Path -Parent $PSScriptRoot
$taskPython = 'C:\Users\Ted\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$taskManifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Cannon fix build.json') -Raw | ConvertFrom-Json
foreach ($taskFile in $taskManifest.files) {
    $taskFilePath = Join-Path $taskRepo $taskFile.path
    if ((Get-FileHash -LiteralPath $taskFilePath -Algorithm SHA256).Hash -ne $taskFile.sha256) {
        throw 'The prepared build has changed. No installation performed; ask for an updated installer.'
    }
}
if (-not $CheckOnly -and (Get-Process DARKSOULS -ErrorAction SilentlyContinue)) { throw 'Save and close Dark Souls before installing.' }
$taskArgs = @('tools/framework.py', 'install', '--character', 'mario64', '--asset', 'C:/Users/Ted/Downloads/Super Mario 64 (USA)/Super Mario 64 (USA).z64', '--runtime', 'work/framework-build/characters/mario64/sm64.dll', '--collision', 'work/hazard-cache-v2/combined')
if ($CheckOnly) { $taskArgs += '--check-only' }
Push-Location -LiteralPath $taskRepo
try {
    & $taskPython @taskArgs
    if ($LASTEXITCODE -ne 0) { throw 'Installation checks failed. Read the message above.' }
    if (-not $CheckOnly) { Write-Host 'Mario doorway, pit death, rendering, cannon aiming and loading fixes installed. Start Dark Souls normally.' }
} finally { Pop-Location }
