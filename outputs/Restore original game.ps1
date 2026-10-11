param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$gameDir = 'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA'
$manifest = Get-Content -LiteralPath (Join-Path $gameDir 'MarioSouls\install.json') -Raw | ConvertFrom-Json
$proxy = Join-Path $gameDir 'dinput8.dll'
$movie = Join-Path $gameDir 'movWW\frpg_title.wmv'
$backup = Join-Path $gameDir 'MarioSouls\backup\frpg_title.wmv'
if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -ne $manifest.original_title_sha256) { throw 'The original title backup failed verification. Nothing changed.' }
if ((Test-Path -LiteralPath $proxy) -and (Get-FileHash -LiteralPath $proxy -Algorithm SHA256).Hash -ne $manifest.proxy_sha256) { throw 'The input loader has changed since MarioSouls was installed. Nothing changed.' }
$movieHash = (Get-FileHash -LiteralPath $movie -Algorithm SHA256).Hash
if ($movieHash -ne $manifest.short_title_sha256 -and $movieHash -ne $manifest.original_title_sha256) { throw 'The title movie has changed since installation. Nothing changed.' }
if ($CheckOnly) { Write-Output 'Restore checks passed. No files changed.'; exit 0 }
if (Get-Process DARKSOULS -ErrorAction SilentlyContinue) { throw 'Quit Dark Souls through its System menu, then close the game before restoring.' }
if (Test-Path -LiteralPath $proxy) { Remove-Item -LiteralPath $proxy }
Copy-Item -LiteralPath $backup -Destination $movie -Force
Write-Output 'MarioSouls is disabled and the original title movie is restored. Saves and backups are preserved.'
