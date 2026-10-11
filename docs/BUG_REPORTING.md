# Reporting a bug while playing

Press **F11** while Dark Souls is focused. The mod saves a diagnostic report and
opens a local review page in your default browser. The F8 overlay can stay off.

1. Add a title, what happened, what you expected, and steps to reproduce it.
2. Review the automatically collected context. Edit or remove anything you wish.
3. Choose **Open GitHub issue draft**, sign in if needed, and submit it yourself.

Opening this page does **not** pause Dark Souls. Use it from a safe place when
possible. The shortcut requires a fresh press and has a ten-second cooldown.
Holding F11 while switching windows will not trigger it.

Reports are saved under `%LOCALAPPDATA%\MarioSouls\Reports\`, in separate folders
containing `review.html` and `report.json`. If offline, keep the folder and open
`review.html` later. If the browser cannot open, a message shows the saved folder.
If saving fails, the mod shows the error without opening GitHub.

## What is collected

- Version, Git commit, modified-source flag, and a source fingerprint identifying
  the compiled code.
- Character adapter and host, current/cached area, position, control owner and
  lifecycle generation, and guest-rig availability.
- Native HP/stamina, equipment IDs, guest action/animation and attack state,
  combat and damage-pipeline availability.
- Motion submission/acknowledgement/rejection counts, collision contact and
  terrain counts, native support, last observed scene-target dimensions, focus
  and controller connection status.
- Up to 120 sampled states (roughly 30 seconds during continuous rendering) and
  96 recent events, including transitions, health changes, stamina charges,
  confirmed hits, and rejected adapter/motion frames.

The GitHub draft contains a compact snapshot and six recent events. Detailed
history stays in the JSON file; download it from the review page and attach it
manually if useful. **No files are uploaded automatically.**

No raw logs, player/account names, local paths, ROMs, saves, screenshots, memory
addresses or key/button history are collected. Positions and equipment still
reveal gameplay context, so review reports before sharing them.

The recorder is read-only and uses existing SDK state for every character. It
does not arm the QA harness or change gameplay/control ownership. Sampling stays
in memory; disk writes and browser launching run on a separate worker only after
an explicit F11 press. To disable recording and the shortcut:

```ini
[Diagnostics]
Enabled=0
```

## Limitations

This is a diagnostic snapshot, not an input replay or crash dump. It cannot
recover a report after a crash or explain every missing collision. Owner values:
0 = native, 1 = character, 2 = interaction, 3 = death. Unavailable positions are
`null`; unavailable HP/stamina are -1. Scene targets can differ from display
resolution. Event times are relative to the Windows uptime clock and can wrap.
Health-change events compare the same lifecycle generation; respawns and actor
changes are recorded separately. Confirmed-hit events use the existing feedback
observer, so they may be absent when feedback is disabled.

GitHub requires repository access while this repository is private. Players can
use the same flow once it is public. GitHub limits URL sizes: keep drafts concise
and attach JSON separately. If a large edited draft is refused, copy its text
into a new issue manually. This uses documented
[GitHub issue URL parameters](https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/creating-an-issue).

## Developer checks

`python tools/framework.py test --shared-only` checks bounded history, clock wrap,
focus/cooldown guards, JSON parsing, unavailable/nonfinite positions, HTML
escaping, build identity, Unicode paths, file collisions and write failures.
It also checks that failure logs and JUnit results survive a failed suite.
These checks never access a running game.

A successful build and offline tests do not prove live F11 behavior. After
installation, verify separately on a disposable test character: report with the
overlay off; inspect context; cross an area and report again; check focus and
held-key behavior; confirm that no issue is submitted automatically.
