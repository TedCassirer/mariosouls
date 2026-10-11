# Framework verification — 2026-10-11

## Framework 0.3 body and landing policy

Final installed DLLs were tested directly in PID 22660 on Mario QA, level 6,
Asylum. Exact hashes and measurements are in `work/body-qa/validation.json` and
`checkpoints/framework-v0.3-body.json`. Earlier candidate evidence is retained
separately under `work/body-qa/candidate/`.

- Shared ABI 3 body descriptor: Mario model scale 0.9; movement capsule radius
  22 cm, total height 105 cm (previously 40/150); secondary contact capsule
  24/110 (previously 45/160). Support capsule preserves its original radius ratio.
  Feet remain anchored. Outgoing attack reach, speed and jump constants unchanged.
- Native geometry changes run before Havok in the existing movement hook.
  Actor, movement, physics, owner and capsule type checks guard every write.
  Three live on/off cycles restored original sizes exactly. Offline mock-memory
  regression also covers rapid reactivation before deferred restoration, stale
  actors, invalid shapes and original-body opt-out.
- Mario's private wall/ceiling queries now match the host dimensions. Real adapter
  test: 22 cm wall clearance, walking under a 120 cm ceiling, head-limited jump
  peak 10.5 cm there. Open-floor jump remains 242 cm. Live jump measured 2.42 m
  with no stamina debit. An unrelated generated adapter also passes ABI 3 checks.
- Final live controlled falls: 11.999 m, HP 594 -> 589 (5 damage); 24.999 m,
  HP 589 -> 560 (29 damage). Exactly one landing damage event per fall; no HP
  restoration. Landing damage scales to 5% of its former value, rounds up, caps
  at 5% of maximum native HP rounded down, and cannot reduce current HP below 1.
  The 1 HP floor is covered by policy tests, not a manufactured live low-HP save.
- Enemy push retained minimum horizontal separation 0.670 m, down from 0.850 m
  in 0.2. Closed main door stopped Mario at Z=-3.604 m. Both remain solid.
- Final incoming melee: one 80 HP hit occurred during native setup; while Mario
  owned movement, HP 514 -> 429 produced Mario hurt and knockback. All sampled
  NoDamage/NoDead/NoHit and other protection flags remained zero. Reduced landing
  damage does not grant jump invulnerability or reduce enemy damage.
- Bonfire rest healed 560 -> 594 after falls, and 429 -> 594 after melee; smaller
  geometry persisted through menu/leave, followed by a successful jump. Final
  save at bonfire, native menu quit, normal process closure. Test transports
  disarmed. No motion rejection, bridge timeout or adapter failure in final log.
- Full supported doctor/build/test workflow passed, including installer transaction,
  core, feedback, motion, body lifecycle, input, actual Mario and unrelated adapter
  checks. Build/runtime/installed hashes match; no replacement build substituted.

Limits: native capsule sizing is not a per-bone hitbox rewrite. Incoming melee
remains functional, but exact avoidance margins for projectiles, grabs and boss
attacks have not been mapped. No fresh other-area, death/respawn, ladder, Estus,
NPC-dialogue, object-break or physical-controller check is claimed. Native death
volumes and bottomless pits remain dangerous; falls during native-controlled
scripts use native rules. Heights beyond 25 m were not exercised in game.

Test-selection incident: during the candidate launch, queued confirmation inputs
unexpectedly loaded the level-65 slot. It was immediately quit normally, before
movement, Mario activation or gameplay tests. The main slot's playtime advanced
36 seconds; no deliberate HP/inventory/save edits were performed. The user was
informed. All remaining load steps were observed individually and all gameplay
checks used the strict Mario QA guard. Final build load selected Mario QA directly.
Save backup: `work/um-state/backups/dark-souls-before-mario-body/20261011-065059.zip`.
Pre-change runtime backup: `work/checkpoints/before-framework-20261011-065754-086026`.
Final install backup: `work/checkpoints/before-framework-20261011-070718-675130`.
Checkpoint: `checkpoint-2026-10-11-framework-v0.3-body`.

The following sections describe earlier versions.


## Framework 0.2 movement ownership

Final installed host and adapter were live-tested directly in PID 27264 on Mario
QA, level 6, Asylum only. The runtime hashes are retained in
`work/ownership-qa/validation.json` and the checkpoint manifest. No PE-timestamp
normalization was used to substitute another build's evidence.

- Full offline suite passes: five installer transactions, core combat, feedback,
  controller/input routes, Mario adapter and independent generated adapter.
  New motion tests cover single consumption, backpressure, stale requests,
  ownership generations, finite bounds, tick counts at 30/60/120 render Hz,
  input latching, velocity clipping, actor lifecycle and timed action return.
- Adapter regressions verify collision correction without animation advancement,
  coherent mesh translation after origin rebasing, and preservation of Mario's
  committed stomp bounce across an acknowledgement with no contact normals.
- Final binary: native quit/load released the old actor, retained the requested
  character, and recreated Mario after the new native actor settled. No F6 or
  activation command was sent after reloading. Observed new actor pointer and
  `SESSION character resumed after stable native actor` in the log.
- Before and after reload, ordinary jumps reached 2.42 m with no stamina cost;
  punches cost 9 with the existing equipment penalty. Four native/Mario toggle
  cycles preserved position and restored control. Bonfire rest healed native HP
  to 594; Leave resumed the same instance and a subsequent jump worked.
- A 75-tick push against the closed Asylum main door stopped at Z=-3.370 m.
  A separate 75-tick push into hollow 1810101 retained at least 0.850 m horizontal
  body separation. Enemy contact supplied velocity-clipping normals; idle
  contact remained stable. Camera-reset setup matters: early attempts moved in
  the previous camera direction and were not counted as door tests.
- Final punch: native hollow HP 44 -> 0; one confirmed impact after 31 ms,
  34 ms requested attacker pause, sound/flash, one stamina debit of 9. Native
  duplicate action debit was suppressed. No damage or balance values changed.
- Final incoming melee: a setup hit happened before activation; while Mario owned
  locomotion, native HP 460 -> 380 and then 380 -> 292 each produced a hurt
  transition and knockback, followed by idle. The second hit landed during the
  retreat setup and is in the game log, outside the timed sample array. No HP,
  protection, inventory, AI or save edits manufactured these results. All sampled
  protection flags remained zero. Bonfire healing restored 594 afterward.
- No rejected movement request, bridge timeout or adapter failure appeared in the
  final log. Submission/acknowledgement count differences at ownership changes
  are intentionally discarded outstanding work, not replayed motion.
- The earlier candidate (PID 40448) also passed minimize/restore without movement
  drift. That check was not repeated after the adapter rebase/bounce fixes.
- Saved through the native quit menu and closed normally. Test transports are
  disarmed in the checkpoint. Level-65 character was never loaded.

Limits: this improves ownership and correction, not every engine incompatibility.
The native position is sampled after consumption of a movement request; this is
not a separate Havok post-solve hook. Static collision caches and short-lived
native contact patches still need broader area coverage. The shared collider is
still the native human body. Large characters need an explicit collider extension.
No fresh death/respawn, other-map, ladder, dialogue, Estus, boss, projectile or
object-break test is claimed. Controller absent; physical-controller testing was
not repeated. Existing historical evidence below does not replace those checks.

Local evidence: `work/ownership-qa/` snapshots, logs and test scripts;
`work/framework-build/live-smoke.json`. Save backup:
`work/checkpoints/saves-before-ownership-20261011-062358.zip`.
Final install backup: `work/checkpoints/before-framework-20261011-063434-313087`.
Pre-change installed impact build: `work/checkpoints/before-framework-20261011-062409-453544`.
Checkpoint: `checkpoint-2026-10-11-framework-v0.2-ownership`.

The following sections describe earlier versions.


## Impact feedback addition — 2026-10-11

Final installed build was tested directly in PID 41044, on Mario QA level 6,
Asylum only. Native HP was 594 throughout. The level-65 character was not loaded.

- New feedback executable tests pass: native HP confirmation, lethal/unchanged/
  healed/missing/recycled/expired targets, clock wrap, one envelope per attack
  serial, weighted bounded durations, buffered inputs, cancellation, motor mixing
  and stereo PCM amplitude/tail bounds. Existing installer, shared combat, input,
  actual Mario adapter and independent SDK fixture tests also pass.
- Punch test with temporary FixedPower=35: native hollow 1810101 HP 44 -> 29;
  one sound, one impact receipt, confirmation after 16 ms, requested pause 34 ms.
  Native attack power was 22.75; stamina cost 9 with the existing equipment cost.
  The subsequent out-of-range punch did not generate an impact.
- Ground pound against stationary hollow 1810108: native HP 22 -> 0; one sound,
  one receipt after 31 ms, requested pause 67 ms, stamina cost 31. Native power
  172.425 at STR/DEX 11, upgrade 0. The native death yielded 24 souls later.
  That confined-cell camera was too close for useful presentation footage;
  the light-hit recording is the visual evidence, and heavy routing is log evidence.
- Visually inspected frame sequences from the final light hit: the gold/white
  burst appears at contact, the short held pose is visible and the native prompt
  stays above Mario. The first, smaller burst was too hidden and was revised.
- Game-process audio was captured separately and muxed into the 7.73-second
  1280x720 proof clip. Raw recording peak 0.3763, RMS 0.0365; no clipped samples.
  This confirms output/capture, not a user's subjective audio preference.
- Bonfire rest/leave still works; afterward, a missed punch produced no feedback,
  and a navigation jump rose 2.47 m with stamina staying at 93. Saved at the
  bonfire through the native quit menu and closed the game normally.
- FixedPower restored to 0, test controls disarmed. No HP, inventory, AI or save
  values were written to manufacture hits. Guarded native repositions used
  living test actors and floors observed in the game.

Test setup limits: two early ground-pound attempts used the slow external test
bridge; the crouch request arrived after landing. Physical keyboard timing fixed
that. The mobile hollow then left the attack radius, so heavy validation moved
to a stationary hollow fixture. HP changes outside our submitted attack receipts
were not counted as feedback successes. No block, boss, object-break or incoming
damage regression is claimed for this addition. Controller absent during the
session: motor mixing/expiry is implemented and unit checked, physical rumble
remains unverified. Full-area grouping is policy-tested, not a fresh live crowd test.

Evidence: `work/impact-feedback/{punch,cell,whiff}.log`, snapshots and captures;
user clip `outputs/MarioSouls impact feedback.mp4`. Final pre-install backup:
`work/checkpoints/before-framework-20261011-060723-523810`; original 0.1 binaries
before any feedback changes: `work/checkpoints/before-framework-20261011-060249-196817`.
Fresh save backup: `work/um-state/backups/dark-souls-before-impact-feedback/20261011-060237.zip`.

The sections below describe the earlier framework extraction pass.

## Automated checks

- Shared combat policy: ABI/profile bounds, finite values, distinct combo windows,
  one debit per attack, exhaustion, per-target limits, automatic attack airtime
  limits, progression, fall policy and rejected mesh/state output.
- Controller mapping: directions in SDK coordinates, deadzone, analog speed,
  diagonal cap, buttons, hold-to-toggle, focus, reconnect and clock wrap.
- Native input routes: movement suppression, preserved camera, ladder/menu input,
  bounded source-specific interaction pulses, cancellation and mouse buttons.
- Actual Mario DLL with the user's ROM and pinned runtime: initialize, version
  rejection, floor, geometry, movement, 242 cm jump, every semantic pose,
  same-instance resume, bounded audio, hurt callback and destroy/recreate.
- Independently generated `sdk_probe` adapter compiled and loaded by the same
  runner with no host changes (1 triangle, 41 cm test jump). This is a synthetic
  SDK fixture, not a second completed character port.
- Five isolated installer tests: read-only check, reinstall when runtime is
  already at the destination, running-game refusal, untested binary refusal and
  rollback after a partial write.
- Architecture guard rejects guest runtime imports in host/core/SDK and common
  direct host-memory access in adapters. Dependency hashes verified.

Assertions are explicitly enabled in the C++ tests despite optimized builds.
The offline report does not claim game verification.

The final host, adapter and runtime were compared byte for byte with the DLLs
used for the live pass, normalizing only PE header/debug/export timestamps.
All three match. The final build passed the offline suite again; its build/test
hashes and the timestamp-only comparison are retained in `work/framework-build/`.

## Observed in the actual game

Loaded **Mario QA**, level 6, Northern Undead Asylum, from the third displayed save
slot. Verified the loaded name/map/level before inputs. The level-65 save was not
loaded or tested. Saves were backed up before installation. Game exited using
its native Quit Game menu after testing.

- Framework ABI 1 and Mario adapter loaded; graphics, world motion, native attack,
  input and action hooks initialized. The native human body remained hidden.
- Picked up the Dungeon Cell Key with Mario active. Item text and prompts rendered
  in front of Mario. Dismissed the popup through native controls.
- Navigation jump moved the actual native body about 2.40 m in the cell and
  2.42 m after leaving the bonfire. Stamina remained 93.
- First punch consumed 9 stamina (8 base, rounded up for the existing 1.00
  equipment-weight penalty). Recorded power 67.925, Strength/Dexterity 11/11,
  upgrade 0. Native duplicate 20-stamina debit was suppressed.
- A closed cell door blocked forward travel. Interaction used the actual key,
  opened the gate and resumed the same adapter instance. No inventory/quest writes
  were used to manufacture the result.
- Punched hollow 1810101: observed HP 44 → 0, a single framework hit record, native
  death and a later native reward of 20 souls. Native return 22 is recorded as a
  return value, not interpreted as HP damage.
- Lit the courtyard bonfire, rested, left, then jumped and punched successfully.
  Logs show `Action complete; same character=0 resumes immediately`.
- Recorded a 30-second silent regression clip showing the bonfire/menu transition
  and return to locomotion. Inspected the contact sheet.

Local evidence: `work/framework-build/live-smoke.json`, `live-combat.json`,
`after-combat.json`, game log copy and `regression.mkv`. User-facing clip:
`outputs/Souls Character Framework regression.mp4`.

Test setup failure: an early native reposition used 184.74 m at a raised doorway
whose floor is about 185.25 m. The body fell below the map, so adapter activation
correctly refused the unsafe floor. Repositioned the living QA character above
the known starting-cell floor and proceeded with physical movement to the door.
This was not counted as a passed collision/hazard check. Future test fixtures must
place the native capsule above a verified floor.

## Scope limits

This is a framework regression pass, not a repeat of all earlier combat-balance
and interaction experiments. Fresh NPC dialogue, Estus, ladder, incoming melee/
projectiles, healing, environmental damage, lethal damage and boss balance were
not revalidated in this pass. The earlier checkpoint evidence remains historical,
not new proof for every scenario. Mario QA has no Estus yet. Input mapping/routing
passed executable tests; physical-controller actions were not manually exercised.

No Link or DK assets, animations or runtimes were added. Character swapping is
restart-based. The v1 SDK currently supports one character, one atlas, the shared
native body and melee/contact damage. See the contract before extending abilities.

The legacy harness transport and raw diagnostic fields remain readable, but new
combat events are `character_attack` and `character_contact`. Historical scripts
that parse `mario_attack` must be migrated before their reports are relied on.
The new `framework/qa` helpers are the supported basic regression path.
