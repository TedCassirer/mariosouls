# Poise-aware kick knockback (2026-10-11)

Framework 0.9.0: see [the cannon record](CANNON.md) for original guest behavior,
Firelink/Asylum live checks, exact scope and remaining gaps. The launcher is an
optional SDK capability; native HP/collision and the QA identity guard remain.

Framework 0.8.3: see [the kick impact record](KICK_IMPACTS.md) for measured
combo/slide/jump reactions, Asylum Demon exclusion, native damage, test setup
failures and coverage limits. Full framework tests and doctor passed. Native
poise and reaction rules remain authoritative; no enemy motion is forced.

# Bouncing Fire Flower (2026-10-11)

Framework 0.8.2 extends ABI4 with an independently versioned optional projectile
profile. Mario supplies tuning; the DS1 PTDE host owns native animation-rate
leases, projectile identification and map contacts. Core validates profiles and
contact policies. No shared spell parameters, charges or enemy HP are patched.

Owned Steam executable SHA256:
`67bcab513c8f0ed6164279d85f302e06b1d8a53abff5df7f3d10e1d4dfd81459`.
Native BulletFlyState update at 0x1018dc0 and map-hit dispatch at 0x10258b0
are signature-checked as a pair. The map wrapper, rather than its inner impact
helper, must be bypassed for a bounce because the wrapper also retires the shot.
FlyState elapsed time resets on pool reuse; its initialized byte does not.
The final implementation clears reused entries on elapsed-time reset, including
when the new occupant is an unrelated projectile. Earlier prototypes that retired
on adjacent floor triangles or missed reused bullets are not counted as passes.

Live checks used only Mario QA, level 13, Northern Undead Asylum, with the user's
current control handoff. Final installed build: 20261011-154042-263847.

- Right-hand cast: windup/release completed in about 0.91 seconds. Three floor
  bounces at 0.167, 0.667 and 1.033 seconds preceded a vertical wall impact at
  1.067 seconds. Positions show ascent and descent between contacts.
- Next shot reused the native pool and bounced before hitting hollow 1810103:
  target HP 69 to 0, displayed damage 134, native reward +24 souls (144 to 168).
  Player HP stayed 594 and stamina 93. The shot consumed exactly one Fireball use.
- Left-hand Flame with a right-hand Claymore: accelerated native 6204/6404 cast,
  two floor bounces followed by impact on a steeper obstacle. One use consumed.
- With Mario disabled, left-hand native casting took about 2.73 seconds;
  animation rate remained 1 and the native Fireball received no bounce handling.
- Four consecutive left-hand casts at roughly 1.74-second intervals produced
  four projectiles and exhausted the four remaining uses. The fifth attempt
  played native empty-cast 6299 at rate 1, with no projectile. Rate returned from
  3.5 to 1 between successful casts. HP and stamina stayed unchanged.
- Right-hand Flame and left-hand Plank Shield restored through native menus;
  charges restored through bonfire rest. The level-65 character was not loaded.
- Three more right-hand casts recorded in the courtyard; final clip contact
  sheet inspected. Mario QA was rested back to eight uses, saved and quit through
  native menus. Exact tested host/adapter hashes match the installed files.
  Verified 92-file runtime archive: checkpoints/framework-bouncing-fireball.json.
  Prior checkpoint archives and tags remain unchanged.

Private evidence: work/bouncing-fireball-qa/final-flight, final-enemy, final-left,
final-native and final-exhaust (recordings, native state traces and logs).
Short game-audio demonstration: outputs/Mario bouncing Fire Flower.mp4.
The final enemy recording confirms the hit but has poor framing from the fixture
warp; it is diagnostic evidence, not a clean showcase. Native camera collision
is unchanged by this feature.

The full framework build/test passed, including real adapter DLL export checks.
Doctor verified pinned dependencies and architecture boundaries. New offline
coverage includes version/size/bounds, floor versus wall/ceiling contacts, adjacent
triangle merging, bounce limits and animation-rate restoration/conflicts.

Scope: ordinary Fireball 4000 / bullet 6000 only. Other pyromancies, all-map slope
coverage, shielded targets, bosses, flame reinforcement scaling, mid-cast death
and free-flight three-second expiry were not revalidated live. These retain
native paths or are bounded in code; they are not claimed as observed passes.
Horizontal aim comes from the native cast; the bouncing projectile deliberately
does not home vertically onto elevated lock-on targets. Fireball keeps native
visual effects and explosions, rather than reproducing SMB sprites exactly.

# Wider character camera (2026-10-11)

Framework0.8.1 adds an optional camera-profile export without changing ABI4.
Mario requests a1.5 distance multiplier. Host changes only live camera distance
parameters; no FOV or view-matrix patch, game param file edit, guest movement
change or character-specific host branch.

Owned executable observation identified repo slot31, LOCK_CAM_PARAM_ST,
38 rows, distance at+0. Stock has two ID200 entries; directory index/ID/offset
are checked instead of relying on unique row IDs. The local Soulstruct
LOCK_CAM_PARAM_ST schema independently describes camDistTarget in metres.

Live Mario QA checks (level13, Asylum, current user control handoff):
- Courtyard: measured native camera-to-target4.000m; Mario5.976m after the
  native interpolation interval; handback4.026m while settling toward4.
- Every one of38 rows was exactly1.5 times baseline during Mario mode and
  restored byte-exact float values after handback, with no accumulated zoom.
- Confined cell/lock-on view: native4.000m, Mario4.087m despite requested6m;
  native scene constraints still brought the camera closer. Screenshots were
  inspected for framing and retained lock-on. This is not an all-map camera test.
- No damage, inventory or stat edits. The level65 character was never loaded.
- Live override1->1.5 changed row0 from4m to6m while Mario remained active;
  the native camera eased outward. Recorded work/camera-qa/demo and inspected
  its contact sheet. Final setting1.5, normal save/quit, verified92-file runtime
  checkpoint: checkpoints/framework-wide-camera.json. Earlier checkpoints remain
  immutable. The installed build is the exact build used for these live checks.

Offline tests cover profile version/size/bounds, repeated updates without
compounding, setting changes, exact restoration, and external-edit conflicts.
The real DLL export rejects incompatible version/size. The full framework
suite passed, including existing combat, input, movement, death and equipment.
Evidence: work/camera-qa/before.json, open/trace.json and confined/trace.json,
with native/character/restored screenshots. camera_smoke.py repeats these checks.

# Fire Flower and native Fireball (2026-10-11)

Framework 0.8.0, optional pyromancy capability on ABI4. Tested only on Mario QA,
the existing exact level-13 fixture, with the native base Pyromancy Flame and
attuned Fireball. Save backup: work/um-state/backups/dark-souls-before-fire-flower/
20261011-142826.zip. The level-65 character was not loaded. The flame/spell kit
was acquired once through the guarded native item routine; native menus handled
equipping and attunement. No HP, projectile, spell-charge or reward writes.

Observed on the final gameplay build:
- Right flame: native windup6304 on lanes6/17, release6504 on lane22; SDK action6
  throughout those stages. Hollow1810103 HP69->0, playerHP594 unchanged,
  stamina93 unchanged, souls96->120 (+24). Native recovery completed before
  the same Mario instance resumed. Charge counter8->7.
- Left flame alongside Claymore: windup6204, release6404, SDK action7. Hollow
  HP69->0, playerHP594 unchanged, stamina93 unchanged, souls72->96 (+24).
  Native charge8->7. Right weapon remained held, not an extra melee attack.
- Real keyboard Tab produced seven further left casts, exhausting the remaining
  uses. The next attempt played native failed-cast6299, emitted no projectile,
  and returned normally. Video covers the last two projectiles and failed attempt.
- Bonfire rest replenished Fireball to8, healed and reset enemies through the
  native game. No extra framework stamina cost was charged for this native spell.
- Switching away from the selected flame restored red hat/shirt and blue
  overalls; selecting it restored the white/red outfit and flower. Hair and
  shoes remain brown. The costume persisted during resting/casting.
- Switching the right weapon to empty retained the left flower and ordinary
  Mario punch. A navigation jump used no stamina (93 throughout), followed by
  return to equipment use. Returning the Claymore allowed its normal swing.
- Protection observations had no global no-damage, no-dead, no-stamina, no-hit
  or player no-dead overrides. Ordinary jumping did not add protection.

The same three-metre rear fixture was first tested with Mario disabled: native
Fireball killed the same69-HP hollow. Earlier cell/rear1.2m attempts consumed
charges and visibly hit scenery/missed, not the target; those are not damage
passes. A front-side setup allowed the hostile hollow to hit Mario before the
cast and one early cleanup failure led to QA's native death (lost284 QA souls).
The helper now attempts retreat without requiring idle first. No save was
restored to hide that result. Fixed setup and successful final traces are separate.

The live comparison exposed distinct left/right native stage IDs. The final
host uses6204/6404 for left and6304/6504 for right; preliminary left casts did
damage but had incorrect visual staging. Fitted throw poses and unchanged-brown
material matching are included in the final tested gameplay build.

Offline build/test/doctor passed: both-hand sockets, finite mesh/capacity,
cast geometry changes without guest hit flags, palette restoration, native
interaction outfit, input-source isolation/cancellation (including held guard),
and existing movement/death/melee/critical/collision suites. Private SDK casting
sheets were inspected using preview-equipment --actions casting.

After a whitespace cleanup, the rebuilt host, adapter and runtime matched the
live-tested binaries byte for byte after normalizing only PE timestamps and
CodeView PDB identifiers. Executable code/data were unchanged. Comparison is in
work/fire-flower-qa/live-build-comparison.json. The final exact build passed the
full suite and was installed after a normal save/quit. The private 92-file
runtime checkpoint was verified against both its archive and installed files.
Its manifest is checkpoints/framework-fire-flower.json. A temporary verifier
mapping initially blocked file replacement; closing its PE mappings resolved the
lock, and the complete final installation/checkpoint verification then succeeded.

Evidence: work/fire-flower-qa/{native-three,mario-three,left-final,right-final,
exhaust}, switch-jump.json, unequipped.png and switch-take.mp4. Final traces,
game logs and game-only audiovisual recordings are kept locally outside Git.

Limits: only base flame + Fireball against one ordinary hollow was damage-tested.
Other spells, reinforcement levels, bosses, resistances, and PvP are not a newly
verified matrix. Physical controller was disconnected; controller mappings passed
executable tests, keyboard Tab and native input pulses were exercised live.
The cast poses use an approximate normalized native clock; projectile origin
and enemy collision retain native human-skeleton behavior. This is not full
hand-to-projectile retargeting. HUD/inventory icon/name remain Pyromancy Flame.
Two-handing, sorcery and miracles are outside this implementation.

# Native parry and critical attacks (2026-10-11)

Framework 0.7.0 adds an optional native_combat capability without changing ABI4
structure layouts. The host submits a bounded native parry/light-attack input,
observes the actual animation lane and clock, and suspends guest attacks and
movement until native recovery. Mario retains his mesh, hand props and authored
shield sweep / thrust / chop. Native eligibility, damage, stamina, paired enemy
animation, invulnerability, deaths and rewards remain authoritative.

Live tests used only Mario QA on the owned PTDE executable. Saves were backed up
before testing. The user-authorized six-weapon kit was granted through native
acquisition, and only Mario QA was raised to the exact level-13 fixture
(STR16/DEX13, other stats11). The user's level-65 character was never loaded.

Final installed-build evidence:
- Plank Shield parry: player lane22 animation485100; enemy lane25 animation2100.
  HP stayed594. Stamina93->79 (14). The same Mario instance resumed.
- Longsword riposte: player lane28 animation233201; enemy lane28 animation2510.
  Target HP69->0, native displayed damage180, stamina93->73 (20), HP594 unchanged.
  Souls236->260 (+24). The paired action completed before same-instance return.
- Longsword backstab: player lane28 animation233400; enemy lane28 animation2500.
  Target HP69->0, native displayed damage136, stamina93->73 (20), HP594 unchanged.
  Souls260->284 (+24). Native recovery completed and Mario resumed.
- Earlier Straight Sword Hilt backstab: target69->63 (6 actual HP), stamina20.
- Failed parry attempts took real incoming damage; parrying is not automatic
  protection. Earlier tests also exercised an ineligible critical attempt that
  resolved as a native ordinary swing instead of fabricating a critical.
- After final backstab, Mario jumped2.42m and performed an ordinary weapon swing.
  Bonfire rest/leave reset enemies and healed through the native game between runs.
- No no-damage/no-dead/no-stamina/global no-hit flags were enabled in the traces.

Native riposte uses32xx on lane28, unlike backstab34xx. A preliminary capture
correctly damaged the enemy but selected the ordinary swing pose; the final
build corrects this and logs kind4 for riposte / kind3 for backstab. Native
sequence lengths vary; the current visual curve is approximate and can return
to its ready pose before the native recovery ends. It never shortens recovery.
Critical camera and enemy alignment retain native human-sized pairing and can
clip around Mario's smaller body; this is not full per-enemy retargeting.

Fixed native parameter lookup: live Behavior/Attack directories contain unsorted
appended groups. Cached, validated row indices replace binary search. This fixed
Battle Axe validation; all six QA kit items were then acquired successfully.

Offline build/test/doctor passed: input source isolation and release, held-input
and focus/actor/device boundaries, native-completion policy, malformed/unsorted
parameter directories, all six families' critical mesh/socket poses, empty-right
shield parry pose, movement parity, death, interaction and collision regressions.
One QA backstab take sent a new file command before activation was polled and did
nothing; it is not a pass. The helper now waits before submitting that command.

Private evidence: work/critical-qa/parry-final and backstab-final2 (trace, log and
game-only video/audio), work/critical-qa/return-check.json. The helper
framework/qa/critical_smoke.py repeats guarded native-AI tests after bonfire rest.

Scope: live critical damage was tested with Longsword and Straight Sword Hilt,
and parry with Plank Shield. Other families passed offline presentation checks,
not a full live enemy/weapon matrix. Physical controller RT was not exercised
in this pass (controller disconnected); routing passed executable tests. NPCs,
bosses, online play, two-handing, empty-hand parries and unarmed criticals are
not newly validated/supported by this change. Unarmed Mario attacks are preserved.

# Weapon animation polish (2026-10-11)

Replaced the shared four-key arm-only sweep with twelve fitted clips across six
weapon families. Authoring fits glove position and weapon orientation together,
projects unreachable goals onto the wrist reach sphere, and retains a bent elbow.
The largest fitted glove-goal error was 0.02cm. Torso twist/lean and head counter-
rotation follow each strike. Contact easing spans the entire arc without an
artificial pause at the middle key. Recovery ends at the identical ready pose.

The actual SDK fixture exported 512 mesh/socket frames with both swings at host
30Hz timings. Front/rear sheets were rendered with native weapon props. These
are offline animation previews, not game captures. A before/after comparison
video is saved under outputs. The framework preview-equipment command reproduces
the sheets using owned assets. All generated geometry stays outside Git.

Full build/test/doctor passed with six-family movement/action parity, finite rigid
sockets, planted-foot mesh checks, collision, interaction and death regressions.
This pass changes character presentation; host timing, stamina costs and damage
remain unchanged. Installed with the transactional installer; previous files are retained at
`work/checkpoints/before-framework-20261011-134722-798738`. The private runtime
archive is verified by `checkpoints/framework-melee-animation.json`.
Live combat checks and the one-time QA arsenal are pending a
current user control handoff. No saves, character levels or inventories have been
changed during this animation work.

# Melee-family extension 0.6.0 (2026-10-11, live testing pending)

Implemented six one-handed families, distinct shared timings, axe/hammer distal
contact and Mario chop poses. Optional capability64 gates new kinds while ABI4
layouts remain unchanged. Equipment-only adapters retain their old contract.

Offline supported build/test/doctor passed. Tests exercise each family's hit
window, once-per-target contacts, cancellation and movement/attachment parity.
Existing interaction, death, collision, input and native block checks also pass.
Prepared and independently audited 82 rigid native props / 574 parameter rows.
Gargoyle Tail Axe model509 is explicitly excluded because its mesh is flexible.

No new live combat result is claimed yet. Installation subsequently completed with the animation pass above; live QA
still awaits a current control handoff. The fixed one-time test kit (Test Mode5) is restricted to Mario QA,
Asylum, living native control, idle/no-menu state, level6 with all eight stats11.
It calls the signature-checked native item-acquisition function for Dagger100000,
Longsword201000, Claymore301000, Scimitar400000, Battle Axe701000, Mace801000, then
sets only STR16, DEX13 and level13. The exact resulting stat tuple is added to the
QA guard; no broad higher-level range is allowed. Level65 remains rejected.
The kit has not yet been invoked or proven in game. No arbitrary grant interface.

Save backup before this work: `work/um-state/backups/dark-souls-before-melee-families/20261011-133406.zip`.

# Mario death presentation (2026-10-11)

Supported full runtime/host/adapter build and test passed. The shared session
checks cover death from locomotion and native interaction, no resume/handoff
from death, no live reactivation while death is owned, identity mismatch and
stable reactivation after release. Mario's actual owned-ROM test plays animation
0x32 through frames 0..95 twice across destruction/recreation. It asserts one
voice callback per death, finite animated geometry, identical terminal meshes for
the remaining 90 ticks, no attack/interaction flags or equipment sockets, and
successful jumping after a fresh life. SDK structs and ABI4 layouts are unchanged.

Installed exact tested host/adapter/runtime; previous contextual-A and equipment
checkpoints remain immutable. Only Mario QA level6 was loaded, with saves backed
up. Native enemy melee took HP594 to0; guest death presentation began at native
zero HP, while native tracks6000/6100 continued. Inspected collapse/final-pose
screenshots and recorded contact sheets: Mario remains visible beneath YOU DIED,
and the game loads and respawns normally. The session log confirms old-actor
release and automatic reactivation after the native actor becomes stable.
Observed respawn HP594/souls0, then a keyboard jump rising2.10m at the sampled
instant. Actual bloodstain recovery restored212 souls; bonfire rest/Leave passed.
Mario QA was saved and the game closed normally with HP594/souls212.

This pass verified melee death. Drowning, kill planes, airborne death, death
during a ladder/menu, controller inputs held across death and F6 mid-death were
not separately exercised live. Those cases use the same guarded ownership path;
no claims of special death variants are made. Adapters without death_pose retain
the previous native death fallback. No health/inventory/reward writes were added.
Private evidence: work/death-qa/melee-trace.json, game.log, screenshots and recording.
Preview: outputs/Mario death animation.mp4.

# Contextual A and interaction stance fix (2026-10-11)

Read-only observation of the reported failure found an active native prompt,
track-14 animation 120000, and no modal/event ownership. The prior idle predicate
accepted only IDs divisible by 100000, silently rejecting this weapon stance.
The host now also recognizes the observed 20000-offset idle bank. Attack, event,
invalid-token and adjacent animation IDs remain rejected in executable tests.

Controller A is assigned once per press: eligible prompt -> native interaction;
otherwise -> guest jump. Menu entry/exit, focus loss, controller changes and actor
replacement cannot recycle a held press into a jump or a second interaction.
Y/Q remain explicit aliases. Space and X/E are unchanged.

Full supported build/test and doctor passed. Live checks used only Mario QA,
level 6, with prior save backups. Native melee killed that character, reducing
212 souls to 0. At the resulting actual "Recover lost power" prompt, the user
pressed physical-controller A and confirmed recovery without jumping. Read-only
observation verified 212 souls restored and HP 594; no reward/HP writes were used.
Bonfire rest/Leave and subsequent keyboard jump also passed, including the
100000 two-handed Hilt idle. Native stance 120000 is covered by the captured
failure and regression test; the user's original character was not driven.
Physical-controller held-A menu transitions are covered offline, not newly
claimed as live tests. Fresh NPC dialogue, doors, ladders and Estus were not
retested in this focused pass.

Private evidence: `work/context-actions-qa/` (death trace, before/after recovery,
bonfire snapshots and game log). The previous equipment checkpoint remains
immutable. The existing native authoritative inventory/reward path is unchanged.

# Framework 0.5.0: equipped combat (2026-10-11)

Installed ABI 4 host, Mario adapter and runtime after the supported build/test
workflow. All installed hashes match the passing report and source receipt.
Prepared 47 private native sword/shield models; no game assets entered Git.
The 0.4.2 checkpoint and corrected Asylum collision cache remain unchanged.
Only Mario QA, level 6, was loaded. The level-65 character was never loaded.
Saves were backed up before testing; Mario QA was saved normally at the bonfire.

Measured in the final installed build:

- Straight Sword Hilt, right hand, with Plank Shield in the left: one sword hit
  changed hollow 1810101 HP 44 -> 31. Native packet selected weapon 212000 and
  right-hand context -1, physical power 20.26, poise 20. Stamina cost was 20 once.
  The hit followed a shield guard, proving the shield did not redirect the
  sword packet to the left-hand item. Confirmed-hit feedback arrived after 31 ms.
- A subsequent swing missed after native knockback moved the enemy out of reach.
  The first combined recording therefore did not prove an unarmed kill. A fresh
  bonfire reset and close-range unarmed test did: HP 44 -> 0, 9 stamina, native
  soul reward, no player HP loss and no false native hurt handoff.
- Rapid ground sword inputs alternated native behaviors 102301000/102301010.
  Stamina fell 93 -> 73 -> 53 -> 33 -> 13; the next attempt was rejected at 13.
  A second exhausted attempt was rejected at 12. No negative attack-cost debit.
- Held guard became the actual native guard state and released cleanly. A real
  right-mouse hold also confirmed native guard without changing stance. Jumping
  from guard retained all 93 stamina and the normal approximately 2.42 m jump.
- Final front-facing shield checks resolved two native blocks, each with 5 HP
  chip and 10 stamina drain. Exact resolved packet observations retained the
  shield pose without triggering SM64 knockback or changing native HP. An earlier
  front attack measured 33 chip/32 stamina; different enemy attacks are not
  treated as a single balance value. Setup/retreat included unguarded hits and
  is excluded from the per-block measurements.
- Rear hits penetrated held guard, cost HP (65 per hit in the observed sequence),
  and did not consume guard stamina. Native passive movement ownership recovered
  after the actual reaction; no health refund or protection flag was added.
- Native bonfire menus, healing to 594 HP, Leave, and subsequent Mario movement
  worked with equipped items. The Plank Shield had been obtained through its
  ordinary pickup earlier in this session. Item-popup field +0x118 was added to
  modal ownership after that pickup exposed a missed popup route.
- The mirrored guest/native coordinate conventions initially put equipment in
  opposite rendered hands. Authored poses and graph sockets were corrected;
  the final screenshot and adapter handedness assertions agree.

The first guard implementation read INI bindings, but native saved options override
them. Live input proved Shift guard versus the INI's Alt value. The final host
reads and validates the active native keyboard configuration each poll. All six
modifier bindings, rebinding, missing config and input route regressions pass.

The first passive-state implementation also paused after an outgoing hit. Native
active-action disable is not sufficient evidence of incoming damage. The final
gate additionally requires an observed HP decrease or an exact native guard-break
receipt. Final sword and unarmed hits caused no such false handoff. Chip suppression
requires the exact resolved block packet, HP interval and one-shot observation;
holding the shield alone cannot suppress a rear hit.

Offline checks passed: installer rollback/asset validation, collision ownership,
shared combat/body/motion/input, swept sword windows, resolved native block packets,
actual ROM-backed Mario movement and animation parity, correct-hand sockets,
6000 armed idle ticks, and an unrelated generated adapter. Controller routing
passes executable tests; no physical XInput controller was connected in this pass.

Not live-verified: actual exhausted guard break (including zero-chip break),
every weapon/upgrade/infusion/status effect, temporary buffs, destructible-object
sword contact, and every incoming hazard. Guard-break packet classification is
implemented and covered by isolated tests, not an observed live guard-break claim.
The incoming HP observation uses a 200 ms latch; unrelated HP loss followed by an
outgoing native action in that interval is a remaining reaction-classification
edge case. Props may disappear during hurt and scripted interaction poses.
Only grounded straight-sword light attacks and small/medium shield blocking are
implemented; parry, two-hand moves and other weapon families need further work.

Private evidence: work/equipment-qa/{safe,front,rear}-trace.json and corresponding
logs, final-attacks-*, final-unarmed-*, final-exhaustion-*, and recorded game clips.
The repeatable guarded input/observation helper is framework/qa/equipment_smoke.py.

# Framework 0.4.2: diagnostic overlay (2026-10-11)

Built and installed the shared F8/F9/F10 debug overlay, then verified it on Mario
QA level 6 only. Exact installed host matches the tested build and its source
receipt. The corrected 0.4.1 Asylum collision cache hash remains unchanged.

- All four views rendered: Combat, Terrain, Objects, All. Terrain displayed its
  3000-triangle cap. Through-walls and depth-tested screenshots visibly differ.
- Live smaller Mario capsules and restored native capsules appeared at the same
  bonfire fixture, with readable labels. Four nearby object origins were counted;
  orange markers were visible facing the main courtyard gate.
- A hollow's native HP changed 44 -> 0 from one punch. The recording shows enemy
  capsule, eligible-point samples, contact/confirmed-hit marks, and the 650 ms
  RECENT attack display. Native soul reward was 20 (152 -> 172).
- Bonfire menu opened, Leave returned to Mario, F8 removed all diagnostics,
  and a subsequent free jump retained stamina 93. Player HP stayed 594 throughout.
  Every sampled protection flag was zero. Screenshots show native prompts/menus
  above diagnostic geometry. Normal save/quit/close completed at the bonfire.
- Offline debug geometry tests cover a large floor triangle whose vertices are
  all out of range, range rejection, NaNs, line-budget clipping, capsule extents
  and invalid capsules. Installer/collision/SDK/body/input/combat suites passed;
  Mario adapter still reports a 242 cm jump and 22 cm wall clearance.

One initial bonfire fixture faced away without a prompt; its interaction request
was correctly rejected and was not counted as a pass. Retested facing the bonfire
with the normal interact key. Teleport fixtures can retain a stale native prompt,
so the hollow recording's prompt is not evidence of bonfire access there.

Private evidence: work/debug-view/ (screenshots, hit recording with separate
metadata and trace JSON, game log, live summary, save backup). No physical
controller, device-loss/reset, cross-map transition or exact native per-bone
hurtbox/weapon-volume mapping was verified. Geometry is diagnostic and does not
change damage or physics. See DEBUG_VIEW.md for precise layer meanings.

---

# Framework 0.4.1: boss contact and event-controlled doorway (2026-10-11)

The user reported that Mario could not damage the Asylum Demon and could not pass
its side exit. Both were reproduced/investigated on Mario QA level 6 only.

- Native boss capsule: radius 2.5 m, endpoints 2.5/5.0 m above its origin. The old
  attack check required the origin inside 1.25 m, even though native collision
  could keep it farther away. Shared contact geometry now queries the body surface;
  automatic downward attacks query the upper surface. Native HP, defense, poise,
  stamina and rewards remain authoritative. No attack power or movement changes.
- Old side doorway test stopped Mario at X=15.260; F6/native movement crossed to
  X=18.276. The static export incorrectly included event-controlled collision
  h0550B1_0000 / entity 1813121, two triangles at X=15.5. The installed event scripts
  normally disable it, then briefly enable it while a gate closes. The new offline
  compiler derives native-owned collider IDs from the installed MSB/event files;
  it removes them from guest static geometry and leaves native Havok/live contacts
  responsible for the barrier. It never writes native event flags or door state.
- New Asylum cache has 65,732 triangles, exactly the old 65,734 minus those two;
  all other triangles are byte-identical. Other maps retain their old caches.
- Fixed centered-door test ran from X=14.0 to X=29.432 and down the stairway,
  Y=198.198 to193.150, while Mario remained active. An earlier off-center candidate
  ended at the right jamb and was not counted as a successful passage.
- Boss combo at initial center separation 3.092 m: HP813->794->775->714, one
  confirmed contact per punch/kick. Stamina93->84->75->58 (9,9,17 including the
  existing equipment modifier). Confirmed feedback31/32/32 ms, pauses34/34/48 ms.
- Separate jump kick: boss813->767,19 stamina,31 ms confirmed impact. Boss then
  damaged Mario594->330; native hurt reaction observed, no extra invulnerability.
- Hollow1810101:44->0 from one punch, one31 ms confirmation; native reward20 souls
  (132->152). First fixture attempt saw the target still streaming and aborted;
  the retry explicitly waited for a living target before attacking.
- Closed main door still stopped Mario at Z=-3.616. Free jump sampled2.40 m with
  stamina93 throughout; offline guest test measured2.42 m. Bonfire healed to594.
- Seven isolated installer tests, three event-ownership tests, capsule geometry,
  body/lifecycle/input/feedback/combat checks and real Mario adapter tests passed.
  The unchanged guest runtime retains22 cm radius,105 cm height,0.9 model scale
  and5%/nonlethal ordinary landing damage policy. Falls were not retested live.

Final install followed normal Quit Game/close at the bonfire, full594 HP. Transports
are disarmed. The final rebuilt host/adapter are byte-identical to the live-tested
DLLs after excluding only PE build timestamp fields; the final offline suite passed.
Private evidence and exact hash comparison: work/contact-fix/. No physical controller,
other areas, every move, boss stagger balance, or per-bone hitbox accuracy is claimed.
Contact remains capsule-based; native damage rules still decide whether damage lands.
Repeatable guarded fixtures are in framework/qa/contact_smoke.py (door/boss), with
read-only native contact inspection in contact_probe.py. Boss stage requires the
existing boss already on the arena floor; it does not spawn or alter enemies.

---

# Framework verification â€” 2026-10-11

## Framework 0.4 â€” Wind Waker Link first playable slice

The final host and Link adapter were tested directly in PID 45932, using Mario QA
level 6 in the Asylum. Candidate PID 43724 evidence is kept separately. Both loads
selected Mario QA from observed menu screens. The level-65 slot was not loaded.

- Original GZLE01 body, sword/hilt, shield, hand variants, bottle and textures;
  25 clips / 738 sampled skeletal frames in a private 6.6 MB asset pack. The
  controller is authored, not an embedded TWW engine or exact physics port.
- Shared SCMESH01 parser/skinning, neutral static collision queries, opt-in
  texture multiplication and alpha cutouts. Host rendering has no character-ID
  branch. Existing rendering-before-UI is retained and visible at the bonfire.
- Final offline suite: installer transactions, common combat/feedback/motion/body/
  input, actual Link and unrelated scaffold contracts, all real animation frames
  bounded, corrupt asset rejection, buffered combo phases, radial spin, roll
  interruption and small-body wall/ceiling tests. Mario's unchanged adapter also
  passed its regression suite with the new host rendering source.
- Final live run/jump/roll and combo/spin recording. Free jump about 0.78 m; roll
  and jump traces have zero sampled protection flags. Sword combo costs 14,16,22
  native stamina, separate serials, 93 -> 41. Spin spends 30. No implicit jump or
  roll protection is added. Physical controller absent during this pass; mapping
  is covered by executable tests, not claimed as a fresh live pad test.
- Final closed main-door push stopped at Z=-3.604 m, no rejected motion requests.
  Hollow1810101 HP44 -> 0 with one hit, one confirmed impact after31ms, 40ms
  attacker pause, native24-soul reward (108 -> 132). Raw native return2 is not an
  HP value. Attack power88.825 entered the native damage path. This is a small
  unarmored enemy test, not a full sword/armor/boss balancing pass.
- Final incoming melee while Link owned movement:464 -> 399 -> 265, native hurt
  callbacks and no protection bits. Earlier damage during setup was not counted
  as owned-state evidence. Returned safely; bonfire healed265 -> 594, Leave
  resumed the same instance and a subsequent jump worked. Original crouch clip
  presents the bonfire rest; the initial candidate's unsuitable standing clip
  was replaced. Candidate repeated tiny landing animations near a contact seam
  were addressed by requiring meaningful downward speed for the landing reaction.
- Game saved at courtyard bonfire and closed normally; test transports disarmed.
  Exact build/installed hashes and traces live under `work/link-ww/`; source and
  local runtime checkpoint: `checkpoint-2026-10-11-framework-v0.4-link`.

Scope limits: shield blocking, target lock/parry, projectiles, guest voices/SFX,
automatic edge jumps and facial-animation playback are not implemented. Native
interaction presentation includes ladder, door, talk and drink clips, but those
four paths have only offline pose coverage in this Link pass. Mario QA has no
Estus yet. No fresh destructible, other-area, boss, death/respawn, projectile,
environmental-hazard or fall-damage testing is claimed for Link. Link retains
native fall rules; Mario's existing reduced-fall profile is unchanged.

Private originals/extracted assets/captures are excluded from Git. The local
pack is required separately from the source checkpoint. Prior Mario checkpoint
tags and archives are unchanged.

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


## Impact feedback addition â€” 2026-10-11

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
- Punched hollow 1810101: observed HP 44 â†’ 0, a single framework hit record, native
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


### 2026-10-11 pit regression correction (prepared)

The earlier statement that bottomless pits remained dangerous was not proven and
is superseded: Firelink's death-camera collision was imported as solid terrain,
preventing Mario from reaching the kill plane. The new host/cache fix distinguishes
camera and lethal filters; protected landing damage remains unchanged. Offline
geometry and actual guest fall checks are separate from live native death/respawn,
which has not been exercised for this prepared revision. No game controls used.


### 2026-10-11 seamless area and unlabelled-enemy regression (prepared)

Video and installed-build logs reproduce repeated Firelink/Burg map-ID changes
releasing the guest and waiting for native idle. Read-only live observation found
an alive c1201 enemy whose script ID was -1; both melee loops excluded such actors.
Native disassembly confirms D78660 resolves script IDs then calls DEA8E0 with
actual source/target pointers. The prepared host uses that builder directly.

Offline regression checks cover same-actor map rebinding, retained ownership,
real actor/physics replacement, separate hit windows for two enemies with script
ID -1, stale/recycled native handles, signature refusal, and the production x86
thunk's registers/stack/return convention against a fake callee. These are not
live damage measurements. The user requested preparation only; this build has
not been installed or tested in-game. Running crossings, actual HP loss, weapon
hits, stagger and soul rewards across the seam remain live checks.
