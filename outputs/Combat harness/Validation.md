The first comparison harness is ready. It resets the selected hollow between attempts, measures actual HP losses, records the configuration and inputs, and stops on unexpected state. No progression, damage, or stamina values were tuned.

Installed plugin: `0.4-experimental-combat-harness`.
SHA-256: `65521e2d5394da05d51e8d26a6370574bd859f4a72c3683563fee27a0492a5e1`.

| Check | Observed result | Evidence |
|---|---|---|
| Three identical punches, each after a reset | 14, 14, 14 HP lost; one observed decrease each | [Final comparison report](20261010-230921/Report.md) |
| Out-of-range punch | 0 HP lost | [Final comparison report](20261010-230921/Report.md) |
| Sustained attack-button input | One 14-HP decrease; enemy moves away afterward | [Final comparison report](20261010-230921/Report.md) |
| Actual focus switch | `focus_lost`, native controls released, no further synthetic frames | [Focus result](20261010-230921/05-focus_loss-result.json) |
| One ordinary incoming hit | Exactly one 45-HP decrease | [Incoming report](20261010-230626/Report.md) |
| Repeated incoming hits and hurt recovery | Three losses: 79, 45, 50; two recorded hurt-to-idle recoveries between them | [Incoming trace](20261010-230626/01-incoming_repeated-events.jsonl) |
| Native healing | Ordinary bonfire rest restored 549 to 594 HP | [Healing result](20261010-230626/02-incoming_heal-result.json) |
| Native death and respawn | 594 total HP lost, final sample 0, stop on death, normal respawn at 594; enemy remained at 69 HP | [Death result](20261010-230626/03-incoming_lethal-result.json) |
| Cancel during a 150-frame command | Stopped before completion; native controls released | [Cancel result](20261010-230626/04-cancel-result.json) |
| Missing heartbeat | Stopped with `lease_expired` | [Watchdog result](20261010-230626/05-lease_expiry-result.json) |
| Kill and ordinary encounter reset | Four observed decreases totaling 44; bonfire reset reacquired the same game ID alive at 44 HP in its original behavior | [Kill/reset result](20261010-225539/10-kill_reset-result.json) |
| Punch through a wall | Verified in-range blocking surface; 0 HP lost | [Wall result](20261010-225539/09-punch_wall-result.json) |
| Punch–punch–kick | All three animations identified; 14 HP total, with later strikes missing the fleeing target | [Combo result](20261010-225539/05-combo-result.json) |
| Ground pound | Ground-pound action at the native hit attempt; subsequent 14-HP decrease | [Ground-pound trace](20261010-225539/08-ground_pound-events.jsonl) |
| Existing visible-Mario interaction | Mario remained visible at the bonfire, left normally, and jumped afterward | [Interaction regression](20261010-231141/interaction-regression.json) |

The attack-specific and kill/reset evidence above uses the preceding instrumented build. The final build adds focus-stop handling and reliable terminal-HP sampling; combat damage, range, cooldown and interactions are unchanged. Each dated folder records its exact build and runner hashes, settings and starting conditions.

All incoming checks used the known test-protection/debug flags disabled. Only Mario's outgoing damage was suppressed during those armed sessions. The final incoming setup starts farther from the armed hollow so that verification finishes before an attack. Different enemy attack choices and knockback cause different incoming damage totals; those are not damage-formula changes.

The following remain limited or unverified:

- **Stomp:** a jump caused 14 HP loss, but the harness does not yet isolate stomp attribution sufficiently. Its result is inconclusive.
- **Dive:** the tested recipes have not reliably produced a dive in this encounter. Earlier attempts produced air kicks; the latest attempt did no damage. Results remain inconclusive.
- **Sustained physical contact:** holding the attack button produced one hit, but the hollow flees. This does not prove repeated-hit suppression under uninterrupted physical contact.
- **Full combo damage:** animation identification is verified; damage from the second punch and kick still needs suitable contact conditions.
- **Controller disconnection:** policy tests passed, but no physical unplug/replug test was performed. No controller was connected during the final runs.
- **Character/map replacement:** guard logic is tested outside the game. Actual death/loading/respawn was exercised; a different character or area was not loaded for testing.
- Environmental hazards, armored enemies and bosses were not added to this first version.

Ten measurement tests passed, including rejection of native-return-only damage, delayed HP confirmation, healing, multiple hits mislabeled as one, and damage attributed to the wrong attack. Guard tests cover identity/loading, focus, controller loss, target replacement, death, menus, invalid input and lease limits. Existing controller and interaction state tests also passed. Restore receipt checks passed without changing files.

Only `test` was used. The level-65 character was not loaded. Saves were backed up and never restored; no enemy HP, AI, death flags, or progression values were written. Earlier failed and inconclusive runs remain in the dated folders for diagnosis.

Testing ended with the harness disarmed, synthetic inputs cleared, and Mario beside the bonfire with full health. [Final game screenshot](20261010-231141/Mario%20ready%20after%20tests_small.png).
