$ErrorActionPreference='Stop'
$taskWork=Split-Path -Parent $MyInvocation.MyCommand.Path
$data='C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA'
if (Get-Process -Name DARKSOULS -ErrorAction SilentlyContinue) { throw 'Exit Dark Souls before installing.' }
if ((Get-FileHash -LiteralPath (Join-Path $data 'DARKSOULS.exe')).Hash -ne '67BCAB513C8F0ED6164279D85F302E06B1D8A53ABFF5DF7F3D10E1D4DFD81459') { throw 'Unsupported game executable.' }
$dll=Join-Path $data 'dinput8.dll'
$cfg=Join-Path $data 'mariosouls.ini'
if ((Test-Path -LiteralPath $dll) -or (Test-Path -LiteralPath $cfg)) { throw 'Existing mod files; refusing to overwrite.' }
$config=@"
[MarioSouls]
Library=$taskWork\build\sm64.dll
ROM=C:\Users\Ted\Downloads\Super Mario 64 (USA)\Super Mario 64 (USA).z64
Collision=$taskWork\collision\m18_01_00_00.bin
FlipWinding=1
RenderMode=1
HideNative=0
DisableMapHit=1
"@
$config | Set-Content -LiteralPath $cfg -Encoding ASCII
Copy-Item -LiteralPath (Join-Path $taskWork 'build\dinput8.dll') -Destination $dll
@{created=@($dll,$cfg);log=(Join-Path $data 'mariosouls.log');dllSha256=(Get-FileHash -LiteralPath $dll).Hash;gameSha256=(Get-FileHash -LiteralPath (Join-Path $data 'DARKSOULS.exe')).Hash} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskWork 'prototype-install.json')
'Installed prototype; activation is manual with F6.'
