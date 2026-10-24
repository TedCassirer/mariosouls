# PTDE behavior and source checklist for Remastered

This checklist describes the changes present in MarioSouls' PTDE implementation and where their code lives. It is a reference for another developer or LLM agent working on a Remastered port. The port author chooses which items are relevant, their implementation order and their verification strategy. No recording, automated game control, particular testing method or evidence-reporting workflow is assumed or required by this checklist.

Reference: PTDE source at **`cb56bb83d4e8e04ee5fd45d4b64da1aa645b8ce6`**, the `v0.9.0-dev.2` preview. The external Remastered implementation has not been inspected here; unchecked items do not mean it lacks those features. The descriptions below concern source behavior, not a guarantee that every interaction has complete gameplay coverage. Existing evidence and limitations are documented in [FEATURES_AND_TWEAKS.md](../../docs/FEATURES_AND_TWEAKS.md).

## Where the changes live

- **Mario adapter and prepared libsm64 runtime:** guest movement rules, animation, poses, body-query edits and props are in `framework/characters/mario64/`. The built runtime includes staged modifications; an untouched upstream `sm64.dll` does not contain those changes.
- **Shared core and SDK:** neutral timing, motion, interpolation, combat and presentation policies are in `framework/core/`; contracts and optional profiles are in `framework/sdk/`.
- **PTDE host:** native observations/actions, structures, IDs, collision, input, equipment, rendering and installation are in `framework/hosts/ds1ptde/`. Its addresses, layouts, animation/item/map IDs and D3D9 integration are PTDE-specific, not Remastered bindings.

`character.json` defaults are compiled into the adapter descriptor. Other values come from INI overrides or separately versioned profile exports. A loaded base `SoulsCharacterGetAPI` table does not by itself enable the optional integrations.

The profile path starts in [`profiles.h`](../characters/mario64/profiles.h), then PTDE [`loader.h`](../hosts/ds1ptde/loader.h), `initCharacter`, calls `cameraConfigure`, `projectileConfigure`, `impactConfigure`, `defenseConfigure`, `environmentConfigure`, `launcherConfigure`, `classConfigure` and `unarmedEquipmentConfigure`. Each consumer loads and validates its profile before applying the relevant native integration.

Related references: [ARCHITECTURE.md](ARCHITECTURE.md) for ownership/dependencies, [CONTRACT.md](CONTRACT.md) for units/lifecycle/exports, and [PORT_CHECKLIST.md](../hosts/ds1ptde/PORT_CHECKLIST.md) for the native integration inventory. [REMASTERED_HANDOFF.md](REMASTERED_HANDOFF.md) describes the earlier presentation-only milestone; this checklist also covers the later gameplay work.

## Movement and collision

### 01. Ordinary-floor traction

- [ ] Extra grip applies to `SURFACE_DEFAULT` floors outside `TERRAIN_SLIDE`, with `normal.y > 0.70710678` (shallower than 45 degrees). Runtime preparation changes all three checks: `mario_floor_is_slippery`, `mario_floor_is_slope` and `mario_floor_is_steep`. This addresses sliding onset, slope acceleration and uphill steepness together. The change does not retune flat movement, airborne velocity or deliberately slippery terrain.

**Code:** [`runtime_grip.h`](../characters/mario64/runtime_grip.h), `sc_mario_grip_floor`; [`prepare_runtime.py`](../characters/mario64/prepare_runtime.py), `sc_grip.h` staging/injection; [`build_runtime.py`](../characters/mario64/build_runtime.py), `build_runtime`. The build copies the pinned dependency into staging and applies these edits there.

### 02. Body dimensions, small doorways and stair stepping

- [ ] Model scale is **0.90**. The movement capsule uses **22-cm radius / 105-cm total height**; the secondary contact body uses **24/110 cm**. Model size, collision dimensions and attack reach are separate. Guest body/ceiling queries are adjusted alongside the native capsules. The reduced guest body retains SM64's **30-cm ground step probe**; an earlier height-scaled probe caused stairs to block movement. Native dimensions are captured/restored through actor-bound ownership. These capsules are not per-bone hurtboxes.

**Code:** [`character.json`](../characters/mario64/character.json), `body`; [`prepare_runtime.py`](../characters/mario64/prepare_runtime.py), `mario.c`/`mario_step.c` replacements; [`runtime_body.c`](../characters/mario64/runtime_body.c); PTDE [`body.h`](../hosts/ds1ptde/body.h), `bodyCapture`/`bodyIdentity`; shared [`body.h`](../core/body.h).

### 03. Native motion acknowledgement without animation resets

- [ ] Guest movement produces a bounded motion request. Native physics resolves displacement, and consumption/position observation precedes another guest tick and outgoing contacts. Mario's `resolveMotion` shifts state, mesh and attachments together, clips momentum only into supplied contact normals, and preserves tangential velocity/action/animation. `resume` is used for ownership handback, not ordinary per-frame position correction. PTDE acknowledgement is consumption plus a subsequent position sample, not a separately hooked Havok post-solve callback. Native positions use metres; SDK positions use local centimetres.

**Code:** [`motion.h`](../core/motion.h), `Mailbox`; PTDE [`world.h`](../hosts/ds1ptde/world.h), `characterWorldMotion`, `worldSubmit`, `worldReconcile`, `worldReady`; [`gameplay.h`](../hosts/ds1ptde/gameplay.h), `prepareCharacterSimulation`/`simulateCharacter`; Mario [`adapter.cpp`](../characters/mario64/adapter.cpp), `resolveMotion`, `shiftEquipment`, `resume`.

### 04. Missing floors, event geometry and invisible barriers

- [ ] The open-world runtime handles missing guest floor geometry without stopping horizontal movement or treating the previous floor as an invisible landing. Its sentinel supplies floor data required by guest action code, not collision geometry. Native contact patches are temporary. With native-wall capability, near-vertical static guest walls are omitted while floors/ceilings remain. PTDE terrain filtering excludes NPC barriers, death-camera surfaces and elevator map-group triggers from ordinary terrain. The Asylum floor changes with native event state.

**Code:** [`runtime_open_world.h`](../characters/mario64/runtime_open_world.h) and its preparation; [`terrain_policy.h`](../core/terrain_policy.h), `staticSurface`; PTDE [`world.h`](../hosts/ds1ptde/world.h), `worldHostSurfaces`; [`terrain.h`](../hosts/ds1ptde/terrain.h), `loadMap`/`rebuildSurfaces`; [`hazards.h`](../hosts/ds1ptde/hazards.h), `guestTerrainFilter`; [`arena_floor.h`](../hosts/ds1ptde/arena_floor.h), `arenaRefresh`.

**Limits:** PTDE filter numbers are edition-specific. The optional private Asylum floor asset is not bundled by the current source installer. Some Ash Lake shape exports use native-only fallback; their guest traversal remains unverified in the reference.

### 05. Lifts, moving support and origin rebasing

- [ ] Native physics supplies platform transport. `NativeSupport` tracks support/normal/owner evidence, and contact patches are refreshed after origin rebasing before the next guest step. Upward motion, removed support and ownership changes invalidate stale support. Actor-capsule tangents are excluded from guest floors. PTDE rebases beyond local X/Z **40 m** or Y **50 m**, shifting local coordinates without destroying the guest.

**Code:** [`motion.h`](../core/motion.h), `NativeSupport`; PTDE [`native_support.h`](../hosts/ds1ptde/native_support.h), `grounded`/`read`; [`world.h`](../hosts/ds1ptde/world.h), `worldHostSurfaces`/`worldReconcile`; [`gameplay.h`](../hosts/ds1ptde/gameplay.h), rebase handling and the following `worldHostSurfaces` call.

### 06. Reduced landing damage without pit immunity

- [ ] Ordinary landing damage has **0.05 scale**, a **5% maximum-native-HP cap** and nonlethal landings. Kill surfaces/pits have a separate lethal path. PTDE also handles world Y below **-1000 m** as lethal. Landing reduction does not turn off native death or introduce a second HP pool.

**Code:** [`character.json`](../characters/mario64/character.json); shared [`body.h`](../core/body.h), `landingDamage`, [`hazard.h`](../core/hazard.h); PTDE [`hazards.h`](../hosts/ds1ptde/hazards.h), `hazardObserve`; [`gameplay.h`](../hosts/ds1ptde/gameplay.h), `environmentKill` and world-bound handling.

## Timing, rendering and audio

### 07. Fixed simulation cadence and short input taps

- [ ] Guest simulation stays at **30 Hz**, with native backpressure, at most two ticks of accumulated time, and queued-time/input clearing on protected ownership or focus loss. PTDE consumes at most one guest tick per update; low update opportunities can still slow simulation. `InputLatch` retains short button taps between guest ticks. Higher render FPS does not increase SM64 physics speed.

**Code:** [`timing.h`](../core/timing.h), `SCTiming::Clock`; [`motion.h`](../core/motion.h), `InputLatch`; PTDE [`gameplay.h`](../hosts/ds1ptde/gameplay.h), `simulateCharacter`; [`CONTRACT.md`](CONTRACT.md), simulation/presentation timing. PTDE schedules updates from graphics callbacks; that scheduling is host-specific.

### 08. Mesh, held-prop and native-anchor interpolation

- [ ] Validated simulation/pose results are captured into visual history and sampled at render cadence. Body vertices/normals and held-hand attachments share the interpolation phase. Mesh history is stored relative to the feet; native-anchor history separately smooths displayed body translation. Visual delay is about one guest tick (**33 ms**). First frames, teleports, actor/pose-mode changes, gaps and incompatible topology/materials snap or clear history. Damage sockets and physical positions use current state. Drawing does not step the guest or advance its simulation clock.

**Code:** [`mesh_interpolation.h`](../core/mesh_interpolation.h), `MeshHistory`/`Anchor`; PTDE [`runtime.cpp`](../hosts/ds1ptde/runtime.cpp), `guestStep`/`guestPose`, `visualCapture`/`visualReset`; [`render_observation.h`](../hosts/ds1ptde/render_observation.h), anchor observation/sampling; [`character_presentation.h`](../core/character_presentation.h), `mesh`; [`equipment.h`](../hosts/ds1ptde/equipment.h), `equipmentDraw`.

### 09. Wider camera, correct depth and UI order at different resolutions

- [ ] The camera profile requests a **1.5 distance scale**, applied/restored by the host. Rendering supports distinct scene/UI dimensions and draws Mario, equipment, effects and cannon behind native prompts/subtitles/menus with world depth. Graphics-resource lifetime is separate from guest lifetime. The shared renderer declares world/view/projection, materials and depth/phase explicitly; the current backend implements D3D9 and DSfix composition.

**Code:** [`profiles.h`](../characters/mario64/profiles.h), `SoulsCharacterGetCameraProfile`; PTDE [`camera.h`](../hosts/ds1ptde/camera.h); neutral [`render_api.h`](../sdk/render_api.h), [`character_presentation.h`](../core/character_presentation.h); PTDE [`render_surfaces.h`](../hosts/ds1ptde/render_surfaces.h), [`render_composite.h`](../hosts/ds1ptde/render_composite.h), [`ui_order.h`](../hosts/ds1ptde/ui_order.h), [`render_device.h`](../hosts/ds1ptde/render_device.h). D3D9/DSfix code is not a DX11 implementation.

### 10. Mesh-only native hiding and independent audio

- [ ] Native visibility suppression affects model submissions while the actor's animation/equipment/gameplay updates remain active. Actor draw-enable, opacity and control fields are left separate from mesh visibility. This replaced a visibility approach associated with lost native enemy/boss audio. Mario audio uses its own bounded channel alongside native audio. Visibility restoration checks the current actor/model identity.

**Code:** PTDE [`character_visibility.h`](../hosts/ds1ptde/character_visibility.h), `visibilityCapture`/`visibilityRelease`/`characterVisibilityHook`; [`runtime.cpp`](../hosts/ds1ptde/runtime.cpp), `pumpAudio`; [`loader.h`](../hosts/ds1ptde/loader.h), audio initialization. SDK audio counts are stereo sample frames, not bytes.

### 11. Cutscene-specific presentation

- [ ] Cutscene presentation uses a cinematic player root and camera, including orientation/roll/FOV, separately from normal gameplay observations. Native scripts retain movement. Ambiguous player/camera data retains native appearance. Mario currently uses a neutral pose under the cinematic transform, not full skeletal retargeting. `[Framework] CutscenePresentation=0` disables this path at launch.

**Code:** PTDE [`cutscene.h`](../hosts/ds1ptde/cutscene.h), [`native_cutscene.h`](../hosts/ds1ptde/native_cutscene.h); shared [`presentation_transform.h`](../core/presentation_transform.h); Mario [`poses.h`](../characters/mario64/poses.h).

## Lifecycle, controls and interactions

### 12. Mario class, creation preview and persistent intent

- [ ] Pyromancer is presented as Mario, with an animated preview retained through physique/trait submenus. Native stats/tutorial progression remain; the tutorial axe maps to gloves and the Flame is picked up during the tutorial. Saved class identity requests Mario on loading, death/respawn and actor replacement without granting items to existing saves. F6/both sticks overrides the current life/area; `[Framework] StartingClass=0` disables class replacement.

**Code:** PTDE [`starting_class.h`](../hosts/ds1ptde/starting_class.h), [`class_preview.h`](../hosts/ds1ptde/class_preview.h), [`unarmed_equipment.h`](../hosts/ds1ptde/unarmed_equipment.h); Mario [`profiles.h`](../characters/mario64/profiles.h), `SoulsCharacterGetClassProfile`; [STARTING_CLASSES.md](STARTING_CLASSES.md).

### 13. Seamless areas versus actual actor replacement

- [ ] Class intent, guest appearance and locomotion ownership have separate state. A seamless map change with the same actor/physics/movement refreshes terrain while retaining the guest and paid attack state. Actual actor replacement invalidates native leases, queued motion and target handles. The presentation path can retain Mario while native spawn/action work owns movement. Native restoration is scoped to the actor that owns the saved fields.

**Code:** [`session.h`](../core/session.h), `sameActor`/`changeMap`; PTDE [`gameplay.h`](../hosts/ds1ptde/gameplay.h), [`character_state.h`](../hosts/ds1ptde/character_state.h), [`presentation.h`](../hosts/ds1ptde/presentation.h); [`native_targets.h`](../hosts/ds1ptde/native_targets.h).

### 14. Contextual A, menus, Estus and native footing

- [ ] Controller **A interacts at an eligible prompt, otherwise jumps**. Keyboard Q interacts and Space jumps; **Y only cancels the cannon**. X/E requests the selected native item. Input ownership prevents one press from becoming both interaction and jump across menu/device/focus changes. Menus/text/actions suspend guest locomotion. Native interaction/item dispatch first releases guest motion and waits for grounded/idle native footing: **150 ms** stability with a **1000-ms** preparation timeout. This addresses item animations being cancelled by native falling on stairs/slopes.

**Code:** [`context_action.h`](../core/context_action.h), [`native_action_preparation.h`](../core/native_action_preparation.h), `GroundSettle`; PTDE [`controller.h`](../hosts/ds1ptde/controller.h), [`action_input.h`](../hosts/ds1ptde/action_input.h), [`gameplay.h`](../hosts/ds1ptde/gameplay.h), [`actions.h`](../hosts/ds1ptde/actions.h), `requestAction`/`updateInteraction`.

### 15. Same-instance native actions, door orientation and death

- [ ] Native actions own their path/outcome while Mario supplies talk/rest/climb/drink/door poses. `resume` returns the same guest to the resolved native position with stale momentum cleared. The door pose turns its visible root **180 degrees**, without reversing native travel or logical resume heading. Death is a one-shot guest clip held at its end while native soul loss/respawn continue.

**Code:** PTDE [`actions.h`](../hosts/ds1ptde/actions.h), [`death.h`](../hosts/ds1ptde/death.h); Mario [`adapter.cpp`](../characters/mario64/adapter.cpp), `pose`/`resume`; [`poses.h`](../characters/mario64/poses.h); [`prepare_runtime.py`](../characters/mario64/prepare_runtime.py), staged death-pose style.

## Combat, progression and incoming hits

### 16. Equipped gloves and independent Mario power

- [ ] Normal-path gloves in the **active right hand**, +0..15, enable damaging Mario moves. Inventory-only, left-hand/inactive gloves and infusions do not. Reinforcement is saved through native inventory/blacksmith progression. Power is `100 + 10*upgrade + B(STR) + 0.5*B(DEX)`, with the piecewise curve/clamps in shared combat policy. Equipped-weapon ATK does not determine glove-move damage. Native armor still affects incoming damage; stats do not change Mario's speed/jump range.

**Code:** [`combat.h`](../core/combat.h), `strengthBonus`/`power`; [`character.json`](../characters/mario64/character.json), [`profiles.h`](../characters/mario64/profiles.h), `SoulsCharacterGetUnarmedEquipmentProfile`; PTDE [`unarmed_equipment.h`](../hosts/ds1ptde/unarmed_equipment.h), [`combat.h`](../hosts/ds1ptde/combat.h). Exact curve/config: [FEATURES_AND_TWEAKS.md](../../docs/FEATURES_AND_TWEAKS.md#marios-gloves-and-progression).

### 17. Individual hit windows, stamina and large/streamed targets

- [ ] Punch1, punch2 and the finishing kick have separate attack windows. Each intended attack charges stamina once and hits each target once. Stomps have an exhaustion/per-airborne-cycle guard; navigation jumps have no added combat cost. Contact sampling uses target collision geometry and descriptor ranges/height bands, rather than only enemy feet. Ground pound is limited to **three targets**, without a height bonus. Native packets retain defenses, blocking, poise, deaths and rewards. Eligible moves also damage native destructible objects; automatic stomp excludes objects. This replaces the early ladder-attack mapping and blanket 450-ms cooldown.

**Code:** [`character.json`](../characters/mario64/character.json), all eight attack definitions; [`combat.h`](../core/combat.h), `Window`; Mario [`adapter.cpp`](../characters/mario64/adapter.cpp), `identifyAttack`/`probe`/`contact`; PTDE [`combat.h`](../hosts/ds1ptde/combat.h), [`target_geometry.h`](../hosts/ds1ptde/target_geometry.h), [`native_targets.h`](../hosts/ds1ptde/native_targets.h), [`native_combat.h`](../hosts/ds1ptde/native_combat.h), [`object_combat.h`](../hosts/ds1ptde/object_combat.h). The feature inventory contains the resulting multipliers, costs and poise values.

### 18. Weapons, shields, parries and criticals

- [ ] Supported one-handed families are dagger, straight sword, curved sword, greatsword, axe and hammer/mace, with authored upper-body swings and same-frame hand sockets. Equipped attacks use native weapon construction. LB/L1/right mouse guards with a shield; RT/R2/Tab parries; LT/L2 remains crouch/ground pound. Native eligibility controls backstabs/ripostes and paired reactions. A left Flame is not a shield critical modifier. Two-handing, every weapon family and unarmed criticals are not implemented.

**Code:** PTDE [`equipment.h`](../hosts/ds1ptde/equipment.h), [`equipment_actions.h`](../hosts/ds1ptde/equipment_actions.h), [`equipment_native.h`](../hosts/ds1ptde/equipment_native.h), [`equipment_props.h`](../hosts/ds1ptde/equipment_props.h); Mario [`runtime_equipment.h`](../characters/mario64/runtime_equipment.h), [`equipment_keys.h`](../characters/mario64/equipment_keys.h); [EQUIPMENT.md](../characters/mario64/EQUIPMENT.md).

### 19. Kicks and confirmed-hit feedback

- [ ] Combo/jump/slide kicks request up to **1.40/1.15/1.60 m** native push. Enhanced push requires a native poise break, scales to zero at **80 maximum poise**, excludes radius above **0.80 m** or total height above **2.80 m**, and has a **1.2-s per-target cooldown**. Native reaction/collision decides actual displacement. Confirmed native HP loss drives deduplicated attacker-only hit stop (**34..67 ms**), sound, flash and rumble. Native simulation continues; misses/full blocks do not generate damaging-hit feedback.

**Code:** [`impact.h`](../core/impact.h), [`feedback.h`](../core/feedback.h); PTDE [`feedback.h`](../hosts/ds1ptde/feedback.h), [`feedback_rumble.h`](../hosts/ds1ptde/feedback_rumble.h); [KICK_IMPACTS.md](KICK_IMPACTS.md), [FEEDBACK.md](FEEDBACK.md).

### 20. Poise and bounded recovery, including midair hits

- [ ] Reaction/recovery profiles provide **+20 native poise**, **1.25x** ordinary reaction rate, **500-ms flinch / 1000-ms knockdown presentation limits**, **4x** recognized native knockdown playback and a **33-ms** quiet return policy. The poise bonus applies to native regeneration and incoming clamp. Recovery handling covers the native state that can keep control locked after a visible Mario clip ends, including airborne damage/fall/landing. It uses native completion for recognized finishable recovery; grabs, paired actions, scripts and death are excluded from ordinary shortening. A second real hit can start a new reaction.

**Code:** [`profiles.h`](../characters/mario64/profiles.h), `SoulsCharacterGetReactionProfile`/`SoulsCharacterGetRecoveryProfile`; [`reaction.h`](../core/reaction.h); PTDE [`defense.h`](../hosts/ds1ptde/defense.h), [`native_reaction.h`](../hosts/ds1ptde/native_reaction.h), [`recovery_native.h`](../hosts/ds1ptde/recovery_native.h), [`recovery_hooks.h`](../hosts/ds1ptde/recovery_hooks.h).

### 21. One health pool, statuses and protection

- [ ] Native HP, stamina, healing, armor, statuses and rings remain authoritative. Guest health/death does not apply a duplicate native damage event or replace native death. SM64 action/invulnerability flags, ordinary jumps and visual hit stop grant **no native invulnerability**. Native grabs/protected ownership suspend guest movement and outgoing attacks. Exhaustive status/grab coverage has not been established in the reference.

**Code:** Mario [`adapter.cpp`](../characters/mario64/adapter.cpp), `writeFrameworkState`/`damage`/`step`; [`runtime_body.c`](../characters/mario64/runtime_body.c); PTDE [`gameplay.h`](../hosts/ds1ptde/gameplay.h), HP observation; [`native_reaction.h`](../hosts/ds1ptde/native_reaction.h), [`death.h`](../hosts/ds1ptde/death.h); [CONTRACT.md](CONTRACT.md), ownership.

## Fire Flower, environment and optional presentation

### 22. Fire Flower visuals, both-hand input and bouncing fireballs

- [ ] An active Flame in either hand selects white hat/shirt and red overalls plus a held Flower. Left casting uses **LB/L1/right mouse**, right casting **B/RB/left mouse**; LT/L2 stays crouch. Native attunement, charges, flame reinforcement, collision and damage remain authoritative. The projectile profile requests **3.5x cast rate**, horizontal **1200 cm/s**, gravity **2400 cm/s^2**, bounce speed **600 cm/s**, lifetime **3 s** and spawn height **75 cm**. Eligible floor contacts bounce; other impacts retain native handling. `[Projectiles] Enabled=0` disables the override.

**Code:** Mario [`profiles.h`](../characters/mario64/profiles.h), `SoulsCharacterGetProjectileProfile`; [`fire_flower.h`](../characters/mario64/fire_flower.h), [`flower_keys.h`](../characters/mario64/flower_keys.h); PTDE [`equipment_actions.h`](../hosts/ds1ptde/equipment_actions.h), `EquipmentCastRequest`/`equipmentActionsInput`/`equipmentActionsTick`; [`projectile.h`](../hosts/ds1ptde/projectile.h); shared [`projectile.h`](../core/projectile.h), [`native_action_preparation.h`](../core/native_action_preparation.h).

**Known candidate:** The reference adds **150-ms** native-footing preparation before cast input, with a **1000-ms** timeout and cancellation on invalidated ownership/equipment/device/focus. This was prepared for a reported left-hand cast failure; its effectiveness against that report remains unverified. Cast-input acceptance and the projectile-flight override are separate code paths.

### 23. Lava and damaging enemy-fire reactions

- [ ] Unprotected lava requests the original burning-butt bounce. Orange Charred Ring in either native ring slot suppresses that reaction while native mitigation continues. A fire-bearing hit must lower native HP to request burning run/fall, with a **10-s start-to-start cooldown** and priority over ordinary hurt. Further hits still damage HP; fully blocked hits do not ignite. Lava-ring protection does not suppress enemy-fire reaction. Death, grabs and protected scripts retain priority. No separate damage-over-time or protection is added.

**Code:** [`environment_api.h`](../sdk/environment_api.h), [`environment.h`](../core/environment.h); Mario [`adapter.cpp`](../characters/mario64/adapter.cpp), `environment`, [`runtime_environment.h`](../characters/mario64/runtime_environment.h); PTDE [`environment.h`](../hosts/ds1ptde/environment.h), [`native_environment.h`](../hosts/ds1ptde/native_environment.h), [`native_fire.h`](../hosts/ds1ptde/native_fire.h), [`fire_hooks.h`](../hosts/ds1ptde/fire_hooks.h); [ENVIRONMENT_REACTIONS.md](ENVIRONMENT_REACTIONS.md). The reference documents incomplete live lava/ring/firebomb coverage.

### 24. Cannon, diagnostics and remaining small behaviors

- [ ] The Firelink cannon/Buddy integration uses bounded original guest actions, approximately **5-s opening**, a release-before-fire gate, shared aim-camera/launch heading and native collision on landing. Native prompts have priority. Its runtime includes selected original opening actions, not SM64's full object scheduler. Placement/unlock state is configuration-driven.
- [ ] F6 toggles Mario/native. F8 toggles an **initially hidden** overlay; F9 changes views and F10 changes depth/Xray mode. Capsules, terrain and object origins are diagnostic observations, not complete collision/per-bone hurtbox data. Startup/error logging is bounded at **256 KiB** per launch, without motion/session recording.
- [ ] PTDE's `[MarioSouls] SkipIntro=1` skips logos through a version-checked startup hook. Title/character selection remains; there is no save autoloader.

**Code:** PTDE [`launcher.h`](../hosts/ds1ptde/launcher.h), [`aim_camera.h`](../hosts/ds1ptde/aim_camera.h), [`launcher_render.h`](../hosts/ds1ptde/launcher_render.h), [`debug_observation.h`](../hosts/ds1ptde/debug_observation.h), [`debug_view.h`](../hosts/ds1ptde/debug_view.h), [`startup.h`](../hosts/ds1ptde/startup.h); Mario [`runtime_cannon.h`](../characters/mario64/runtime_cannon.h); neutral [`debug_presentation.h`](../core/debug_presentation.h); [CANNON.md](CANNON.md).

## Removed experiments and edition-specific work

The reference has **no custom equipment-load animation/movement slowdown or added weight-based stamina penalty**. Native armor/ring effects remain. Earlier load-slowdown experiments, Link scaffolds, cheat/developer menus, test labs, command bridges, recording/replay, background control, F11 bug journaling and Steam-free development-copy tools were removed. F1..F4, F7 and F11 have no mod function. They are not part of the retained Mario behavior.

DSfix support, native collision caches, equipment exports, event/fingerprint rules and the installer in this checkout belong to PTDE. Shared code provides contracts, policies and build dispatch; it does not supply Remastered native bindings or a DX11 backend. The existing `build --check-portability` command compiles portable declarations and guest components for x86/x64, while the PTDE host remains x86.

This document identifies the reference implementation and its source locations. It does not prescribe a verification procedure or assess the external port's completeness.
