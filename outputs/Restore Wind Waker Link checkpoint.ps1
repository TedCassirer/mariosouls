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
manifest = json.loads((root / 'checkpoints/framework-v0.4-link.json').read_text())
archive = checkpoint.verify(manifest)
asset = checkpoint.safe_path(root, manifest['owned_asset']['path'])
if not asset.is_file() or checkpoint.digest(asset) != manifest['owned_asset']['sha256']:
    raise RuntimeError('The private Link asset pack is missing or changed. Restore work/link-ww/v1/link.scmesh from your local backup first.')
print('Wind Waker Link checkpoint and private asset pack verified.')
if sys.argv[2] == 'restore':
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        checkpoint.restore(manifest, archive)
    print(captured.getvalue().replace('Restored MarioSouls 0.7.', 'Restored Wind Waker Link / Framework 0.4.'), end='')
"@
$code | & $python - $repo $mode
if ($LASTEXITCODE -ne 0) { throw 'Checkpoint operation stopped. Read the error above; keep the backup.' }
