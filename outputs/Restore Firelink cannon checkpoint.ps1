param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$python = 'C:\Users\Ted\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
if (-not (Test-Path -LiteralPath $python)) { $python = (Get-Command python -ErrorAction Stop).Source }
$mode = if ($CheckOnly) { 'check' } else { 'restore' }
$code = @"
import sys, json, io, contextlib
from pathlib import Path
root = Path(sys.argv[1])
sys.path.insert(0, str(root / 'tools'))
import checkpoint
manifest = json.loads((root / 'checkpoints/framework-firelink-cannon.json').read_text())
archive = checkpoint.verify(manifest)
print('Firelink cannon checkpoint archive verified: ' + str(len(manifest['files'])) + ' files')
if sys.argv[2] == 'restore':
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        checkpoint.restore(manifest, archive)
    print(captured.getvalue().replace('Restored MarioSouls 0.7.', 'Restored Firelink cannon update.'), end='')
"@
$code | & $python - $repo $mode
if ($LASTEXITCODE -ne 0) { throw 'Checkpoint operation stopped. Read the error above; keep the backup.' }
