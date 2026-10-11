MarioSouls combat harness — local experimental version 0.5

The installed combat prototype now has independent Mario power, native stamina costs and attack-specific hit windows. See the parent folder's combat notes for current tuning and validation. Dated 0.4 reports remain historical evidence of the previous implementation.

Double-click **Run combat tests.cmd** to run the short comparison batch. It loads the recognized `test` character if the game is closed, then performs three fresh punches, an out-of-range punch, and a sustained-button check. Keep the game in the foreground and leave its controls untouched while the batch runs. Each attempt rests at the bonfire and verifies the encounter again.

Double-click **Stop combat tests.cmd** to cancel. Losing game focus also ends an armed test. If the runner disappears, its input lease expires within two seconds. Ending a test releases native controls; the game itself continues running.

If rendering is paused, disarming takes effect before synthetic input can resume. The explicit `focus_loss` test briefly switches to an already open ChatGPT window and then returns to Dark Souls; it sends that window no keys or clicks.

Results appear in a new dated folder here. Open `Report.md` for the table, or `results.json` for measurements. Each attempt also has setup observations, a result file, and timestamped frame/action/HP events. Screenshots are limited to failures, inconclusive checks and milestones. Earlier development failures remain visible rather than being overwritten.

Only character **test**, level 4, in Northern Undead Asylum is accepted. The outgoing target is hollow **1810101**, starting at 44 HP. Incoming tests use the nearby armed hollow **1810103**, starting at 69 HP. Equipment and attributes must match `fixture-baseline.json`; a mismatch stops the run. Do not replace that baseline merely to make a failure disappear.

Normal bonfire rest restores health and the encounter. Setup uses the previously verified native positioning method at checked floor locations. Enemies are reacquired by game ID after returning from the bonfire. No enemy HP, AI, death flags, save files, level, or equipment are written. The level-65 character is never selected.

Incoming tests suppress only Mario's outgoing damage for that armed session, preventing incidental contact from killing the attacking hollow. This ends automatically with the session and does not change the normal EnableCombat setting. Incoming damage, AI and defenses stay enabled; the runner checks the known protection flags before each attempt.

For additional checks, run PowerShell in this folder:

```powershell
& '.\Run combat tests.ps1' -Command observe
& '.\Run combat tests.ps1' -Command reset
& '.\Run combat tests.ps1' -Tests 'combo,stomp,dive,ground_pound,punch_wall,kill_reset'
& '.\Run combat tests.ps1' -Tests 'incoming_hit,incoming_repeated,incoming_heal,incoming_lethal'
& '.\Run combat tests.ps1' -Tests 'cancel,lease_expiry,focus_loss'
& '.\Run combat tests.ps1' -Command sequence -Sequence '.\Example punch sequence.json'
& '.\Run combat tests.ps1' -Command stop
```

A custom sequence starts from the checked outgoing fixture. Each JSON step specifies `frames` (1–150 at 30 Hz), `buttons` (A/jump=1, B/attack=2, Z/crouch=4; add values to combine), and `x`,`y` axes between -1 and 1. Directions use the heading captured at session start. At most 60 steps and 90 seconds are accepted. A batch has at most 12 attempts and a ten-minute limit. Exit codes: 0 completed, 1 failed, 2 inconclusive.

The plugin's separate leased protocol supports observe, activate, deactivate, frame-input and stop commands. It is dormant during ordinary gameplay. Manual configuration without a live heartbeat is deliberately insufficient to leave synthetic input running.

HP-loss observations count actual health decreases, not native damage-call return codes. Closely spaced damage may share one observed decrease. New reports show stamina charged by attack events; sampled decreases remain available separately. Attack costs, multipliers, power settings and equipped weight are recorded. Older development builds contained stamina bugs, so consult the current validation notes before using their numbers for balance decisions.

The hollow can flee after a hit. Combo animations can be identified even when later strikes miss; that is not proof of every strike's damage. A wall check requires both an in-range target and a verified blocking surface; otherwise it is inconclusive. See Validation.md for the checks actually demonstrated, remaining limitations, and report links.

These launchers depend on this workspace's `work/combat-harness` sources and installed local Python/Universal Modder runtime. This is a local development tool, not a distributable mod package. The existing Mario movement, controller and visible-interaction implementation remains installed.
