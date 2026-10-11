param(
    [ValidateSet('batch','observe','reset','sequence','stop')][string]$Command = 'batch',
    [string]$Tests = 'punch_in,punch_in,punch_in,punch_out,sustained',
    [string]$Sequence,
    [switch]$ShowResult
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskWork = Join-Path $taskRoot 'work'
$python = 'C:\Users\Ted\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$env:PYTHONPATH = 'C:\Users\Ted\.codex\plugins\cache\universal-modder\universal-modder\0.2.0'
$env:UM_HOME = Join-Path $taskWork 'um-state'
$runner = Join-Path $taskWork 'combat-harness\runner.py'
try {
    $runnerArgs = @('-u', $runner, $Command)
    if ($Command -ne 'stop') {
        $gameProcesses = @(Get-Process -Name DARKSOULS -ErrorAction SilentlyContinue)
        if ($gameProcesses.Count -eq 0 -and $Command -in @('batch','reset','sequence')) {
            & $python -u (Join-Path $taskWork 'fast_test_load.py') --launch *> (Join-Path $PSScriptRoot 'last-launch.log')
            if ($LASTEXITCODE -ne 0) { throw 'The test-character launcher stopped. See last-launch.log.' }
            $gameProcesses = @(Get-Process -Name DARKSOULS -ErrorAction SilentlyContinue)
        }
        if ($gameProcesses.Count -ne 1) { throw 'Expected one running Dark Souls process.' }
        $runnerArgs += @('--pid', [string]$gameProcesses[0].Id)
        if ($Command -eq 'batch') { $runnerArgs += @('--tests', $Tests) }
        if ($Command -eq 'sequence') {
            if (-not $Sequence) { throw 'Supply -Sequence with a JSON sequence file.' }
            $runnerArgs += @('--sequence', $Sequence)
        }
    }
    & $python @runnerArgs 2>&1 | Tee-Object -FilePath (Join-Path $PSScriptRoot ('last-' + $Command + '.log'))
    $resultCode = $LASTEXITCODE
    if ($ShowResult) {
        Add-Type -AssemblyName PresentationFramework
        $message = if ($resultCode -eq 0) { 'Finished. Open the newest dated folder for results.' } elseif ($resultCode -eq 2) { 'Finished with inconclusive checks. Open the newest report for details.' } else { 'Stopped on an unexpected state. Open the newest report or last-run log for details.' }
        [System.Windows.MessageBox]::Show($message, 'MarioSouls combat harness') | Out-Null
    }
    exit $resultCode
} catch {
    $_ | Out-String | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'last-error.log')
    if ($ShowResult) {
        Add-Type -AssemblyName PresentationFramework
        [System.Windows.MessageBox]::Show($_.Exception.Message, 'MarioSouls combat harness stopped') | Out-Null
    }
    throw
}
