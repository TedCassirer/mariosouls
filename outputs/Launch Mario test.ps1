$ErrorActionPreference = 'Stop'
$runtime = 'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\MarioSouls'
$env:PYTHONPATH = 'C:\Users\Ted\.codex\plugins\cache\universal-modder\universal-modder\0.2.0'
$env:UM_HOME = Join-Path $runtime 'tools\um-state'
$python = 'C:\Users\Ted\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$loader = Join-Path $runtime 'tools\fast_test_load.py'
try {
    & $python -u $loader --launch *> (Join-Path $runtime 'tools\last-launch.log')
    if ($LASTEXITCODE -ne 0) { throw 'Automatic loading did not finish.' }
} catch {
    Add-Type -AssemblyName PresentationFramework
    [System.Windows.MessageBox]::Show('Automatic loading stopped. Bring Dark Souls to the foreground and run this launcher again, or select the test character manually. Details are in MarioSouls\tools\last-launch.log.', 'MarioSouls launcher') | Out-Null
    exit 1
}
