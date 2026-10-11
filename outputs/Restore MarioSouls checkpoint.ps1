param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$python = 'C:\Users\Ted\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
if (-not (Test-Path -LiteralPath $python)) { $python = (Get-Command python -ErrorAction Stop).Source }
$script = Join-Path $repo 'work\legacy-checkpoint-tools\checkpoint.py'
if ($CheckOnly) { & $python $script } else { & $python $script --restore }
if ($LASTEXITCODE -ne 0) { throw 'Checkpoint operation stopped. Read the error above; do not delete the backup.' }
