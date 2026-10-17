# Features and tweaks

This is the maintained inventory of MarioSouls behavior and deliberate differences from native Dark Souls or stock SM64. It describes the current development source, not a promise that every entry has passed a full playthrough. [CHANGELOG.md](../CHANGELOG.md) records changes by version; this document records the resulting state.

Update an existing entry whenever its behavior, tuning or default changes. Add a new entry for a new feature, with important values/units, configuration, source locations and verification limits. The complete update rule is in [AGENTS.md](../AGENTS.md).

## Status and evidence

| Area | Current status |
| --- | --- |
| Host | Dark Souls: Prepare to Die Edition, Windows/x86 |
| Character | Mario through libsm64; no other implemented adapter |
| Stock / DSfix | Focused historical game checks with stock rendering and DSfix 2.4 |
| Current automated verification | x86 host/guest compilation; x86/x64 C/C++ SDK/core and x64 guest compilation |
| Remaining game coverage | Full traversal, scripted events, cutscenes, hazards, hardware/configuration combinations and balance need manual checks |
| Remastered | Architecture preparation only; no hooks, DX11 renderer or compatibility claim |

Recent source cleanup was compile-checked. Historical game observations are not new checks of every subsequent revision. Use the [manual checklist](TESTING.md) and report a specific build/area when adding evidence.

## Class selection and persistence

- Pyromancer is presented as **Mario**, including the class name/description and an animated Mario creation preview. Physique and appearance submenus keep the guest preview; those sliders still describe the fallback human rather than reshaping Mario.
- Native Pyromancer stats and tutorial progression remain. The preview weapon and class-specific tutorial axe pickup map to Mario's Gloves. The actual initial native record remains; the Flame is still found during the tutorial rather than granted immediately.
- Saved Pyromancer class identity requests Mario on loading, death/respawn, actor replacement and area changes. Existing Pyromancer saves are recognized without granting equipment or changing stats.
- F6 or holding both stick buttons provides a manual override. Turning Mario off applies to the current life/area; saved-class defaults are reconsidered at lifecycle boundaries.
- `[Framework] StartingClass=0` disables class replacement while leaving manual character mode available.

Sources: [starting_class.h](../framework/hosts/ds1ptde/starting_class.h), [class_preview.h](../framework/hosts/ds1ptde/class_preview.h), [unarmed_equipment.h](../framework/hosts/ds1ptde/unarmed_equipment.h). Details: [class integration](../framework/docs/STARTING_CLASSES.md).

## Movement, body and camera

| Tweak | Default / behavior |
| --- | --- |
| Guest movement | SM64 actions at 30 Hz; native collision resolves attempted displacement |
| Model scale | 0.90; visual size is separate from speed and reach |
| Movement capsule | Radius 22 cm, total height 105 cm |
| Secondary contact body | Radius 24 cm, total height 110 cm; not an exact per-bone hurtbox |
| Landing damage | Scale 0.05, cap 0.05 of maximum native HP, nonlethal ordinary landings |
| Lethal terrain | Native kill surfaces/pits and the host world bound remain lethal; landing reduction does not protect against them |
| Camera | `[Camera.mario64] DistanceScale=1.5` |
| Ordinary slopes | Extra traction on default host floors shallower than 45 degrees; designated slippery/slide terrain retains guest behavior |
| Equipment weight | No custom movement/animation slowdown or weight-based stamina penalty; native armor/ring effects remain |

Stairs, narrow openings, moving support and missing guest-floor data use native movement acknowledgements/support rather than treating a cached Mario floor as final authority. Contact patches refresh after local-origin rebasing; upward motion, removed support and ownership changes invalidate stale support. Seamless map changes retain a valid guest instance while refreshing terrain and target state.

Sources: [character.json](../framework/characters/mario64/character.json), [runtime_grip.h](../framework/characters/mario64/runtime_grip.h), [world.h](../framework/hosts/ds1ptde/world.h), [native_support.h](../framework/hosts/ds1ptde/native_support.h), [hazards.h](../framework/hosts/ds1ptde/hazards.h). Lifts, ledges and scripted floor variants still need location-specific manual coverage.

## Interactions and control ownership

- Controller A interacts at an eligible prompt and jumps elsewhere. Keyboard Q requests interaction and Space remains jump. Y is no longer a world-interaction alias; it only cancels an active cannon sequence. A press consumed by an interaction cannot also jump after a menu closes.
- X/E requests the selected native item, including Estus. Doors, NPC conversations, bonfires, ladders and item use run native scripts with Mario presentation on the same guest instance.
- Interaction preparation settles native footing before dispatch, including slopes and stairs. Menus/text/actions suspend guest locomotion, preventing sliding with an idle or frozen mesh.
- Native-owned movement and animation are mapped to Mario poses. Door facing uses native action orientation; resume clears stale momentum.
- Death has a one-shot Mario presentation while native death, soul loss and respawn remain authoritative.
- Cutscenes have a separate pose-only presentation path using the native cinematic camera and player root transform. This is a neutral-pose first pass, not skeletal retargeting; ambiguous data keeps native presentation. `[Framework] CutscenePresentation=0` disables it on the next launch.

Sources: [actions.h](../framework/hosts/ds1ptde/actions.h), [gameplay.h](../framework/hosts/ds1ptde/gameplay.h), [death.h](../framework/hosts/ds1ptde/death.h), [cutscene.h](../framework/hosts/ds1ptde/cutscene.h), [poses.h](../framework/characters/mario64/poses.h). Cutscene coverage remains pending beyond implementation inspection.

The A/Q-only world-interaction mapping is implemented without changing native-mode bindings. It requires a fresh manual check of prompts, menus and cannon cancellation; compilation alone does not verify input in game.

## Mario's Gloves and progression

Mario's damaging unarmed moves require normal-path **Mario's Gloves in the active right hand**. This is the native Caestus, presented with Mario-specific text, using native inventory and normal +0 through +15 reinforcement. Infusions, left-hand gloves, inventory-only gloves and inactive slots do not enable Mario attacks. Existing characters are not given or auto-equipped items. The inventory icon and displayed ATK remain native fist values.

Vitality controls native health, Endurance native combat stamina, Strength primary Mario scaling and Dexterity secondary scaling. Equipping armor still affects native defenses. Mario's own attack power is independent of equipped-weapon ATK:

```text
Mario power = 100 + 10 * normal glove upgrade + B(Strength) + 0.5 * B(Dexterity)
```

`B(stat)` is zero through 10, adds 3 per point from 10 to 20, 2 per point from 20 to 40, and 0.5 per point above 40. Stats clamp to 1..99; final power clamps to 1..1200. This is pre-defense attack power, not direct HP damage. Movement speed and jump distance do not scale with stats.

`[Combat.mario64]` retains `BasePower=100`, `UpgradePower=10`, `StatScaling=1`, `FixedPower=0` and per-move `<name>_multiplier` / `<name>_cost` tuning. A positive `FixedPower` bypasses the stat/upgrade formula. The equipped glove's saved normal reinforcement supplies the gameplay upgrade level; do not present an INI-only upgrade as a new inventory item.

Sources: [core/combat.h](../framework/core/combat.h), [combat.h](../framework/hosts/ds1ptde/combat.h), [profiles.h](../framework/characters/mario64/profiles.h), [unarmed_equipment.h](../framework/hosts/ds1ptde/unarmed_equipment.h).

## Unarmed attacks and stamina

| Move | Power multiplier | Stamina | Poise damage |
| --- | ---: | ---: | ---: |
| First punch | 0.65 | 8 | 5 |
| Second punch | 0.65 | 8 | 5 |
| Finishing kick | 1.10 | 16 | 30 |
| Jump kick | 0.95 | 18 | 25 |
| Dive | 1.15 | 24 | 10 |
| Slide kick | 1.00 | 20 | 35 |
| Automatic stomp | 0.90 | 16 | 5 |
| Ground pound | 1.65 | 30 | 15 |

Each individual move has its own paid hit window and one hit per target identity. Punches and the finishing kick have separate IDs. Prolonged contact cannot submit damage every frame. Ground pound is a committed area attack, limited to three targets; other descriptors allow up to eight. There is no height-based damage bonus.

Stamina is charged once per attack. An exhausted move has no damaging window and does not receive a free automatic stomp. The automatic attack is limited per airborne cycle to prevent repeated bounce/damage loops. Navigation, ordinary running and jumps have no added combat stamina cost.

Native target defenses, blocking, poise, HP, deaths and souls are retained. Eligible unarmed moves can also submit native damage to destructible objects; automatic stomps are explicitly excluded from object damage. Streamed enemy targets use generation-bearing actor handles rather than optional script entity IDs, so area changes do not leave combat bound to stale enemies.

Sources: [character.json](../framework/characters/mario64/character.json), [core/combat.h](../framework/core/combat.h), [native_combat.h](../framework/hosts/ds1ptde/native_combat.h), [native_targets.h](../framework/hosts/ds1ptde/native_targets.h). Actual HP loss and boss stagger still require measured balance checks.

## Kick knockback and impact feedback

Finishing, jump and slide kicks request at most 1.40 m, 1.15 m and 1.60 m of native push respectively. Requests scale down with maximum poise to zero at 80, require the current poise budget to break, and reject bodies above the 0.80 m radius / 2.80 m total-height cutoffs. A target has a 1.2-second enhanced-push cooldown. Native reactions/collision decide actual travel; these are not guaranteed displacements or a blanket boss immunity rule.

Confirmed native HP loss can trigger attacker-only hit stop (34..67 ms), a procedural impact sound, a brief world-space burst and controller rumble. Feedback is bounded and deduplicated by attack serial. Misses/fully blocked hits do not invent a damaging impact. Native simulation continues through the guest's visual hit stop.

`[Feedback]` defaults: `Enabled=1`, `HitStop=1`, `Sound=1`, `Flash=1`, `Rumble=1`, `Volume=65`, `RumbleStrength=75`.

Sources: [impact.h](../framework/core/impact.h), [feedback.h](../framework/core/feedback.h). Details: [kick impacts](../framework/docs/KICK_IMPACTS.md), [feedback](../framework/docs/FEEDBACK.md).

## Weapons, shields and criticals

Supported one-handed families are daggers, straight swords, curved swords, greatswords, axes and hammers/maces. Their damage uses native equipped-weapon construction, separate from Mario's glove power. Two authored upper-body swings per family preserve guest locomotion and attach extracted native props to the rendered hands.

Shields provide native guard with LB/L1/right mouse and parry with RT/R2/Tab. LT/L2 remains crouch/ground pound. Supported weapon attacks can request backstabs/ripostes when native conditions permit; shield guard+attack also requests a critical attempt. A held left Flame is not treated as a shield-guard critical modifier. Native timing, stamina, protection and paired enemy reactions remain authoritative. Two-handing, every weapon family, unarmed criticals and complete weapon-buff parity are not implemented.

Sources: [equipment.h](../framework/hosts/ds1ptde/equipment.h), [equipment_native.h](../framework/hosts/ds1ptde/equipment_native.h), [runtime_equipment.h](../framework/characters/mario64/runtime_equipment.h). Details: [equipment presentation](../framework/characters/mario64/EQUIPMENT.md).

## Incoming damage, recovery and hazards

- Native HP is the single authoritative health pool. The guest engine's independent health/death effects do not apply a second native damage event. Armor, healing, status buildup and rings continue through native systems; exhaustive bleed/poison/toxic/curse coverage has not been established.
- Mario requests +20 native poise and a 1.25x ordinary native reaction rate. Default one-shot presentation limits are 500 ms for flinch and 1000 ms for backward knockdown/recovery, with 4x native knockdown playback. Safe native completion is still required; grabs, death and protected scripts are excluded from ordinary recovery shortening.
- `[Defense.mario64]` defaults include `BonusPoise=20`, `LightReactionSpeedPercent=125`, `ReturnQuietMs=33`. Poise refills through native regeneration; no jump immunity is added.
- Unprotected lava requests SM64's burning-butt bounce. The Orange Charred Ring suppresses that reaction while native damage mitigation remains native. A damaging enemy fire hit requests the burning run/fall before ordinary hurt presentation, with a 10-second start-to-start cooldown. Further hits still deal native damage.
- Lava/ring comparisons, real firebombs, mixed damage and enemy grabs need focused live verification. Animation alone is not evidence of protection.

Sources: [defense.h](../framework/hosts/ds1ptde/defense.h), [recovery_native.h](../framework/hosts/ds1ptde/recovery_native.h), [environment.h](../framework/hosts/ds1ptde/environment.h), [environment_api.h](../framework/sdk/environment_api.h). Details: [environment reactions](../framework/docs/ENVIRONMENT_REACTIONS.md).

## Fire Flower and bouncing fireballs

An active Pyromancy Flame in either hand gives Mario a white hat/shirt and red overalls, plus an authored held Fire Flower. Switching away restores his normal colors. The inventory/HUD still uses the native Pyromancy Flame name/icon.

Attuned Fireball uses native spell charges, flame reinforcement, collision, damage and rewards. The projectile profile requests 3.5x casting rate, 1200 cm/s horizontal speed, 2400 cm/s^2 gravity, 600 cm/s bounce speed, 3-second lifetime and 75 cm spawn height. Eligible floor contacts bounce; other impacts retain native handling. `[Projectiles] Enabled=0` disables the projectile override.

Left-hand casts use LB/L1/right mouse; right-hand casts use B/RB/left mouse. RT/Tab no longer casts a left Flame. Cast and parry inputs have independent press edges, with no held-button repeat across native handback or equipment changes. A left cast requests the native left-primary pulse, rather than fabricating a projectile or subtracting spell charges. Existing grounded/idle/native-action eligibility remains; casting during a ground pound is not added.

Sources: [equipment_actions.h](../framework/hosts/ds1ptde/equipment_actions.h), [gameplay.h](../framework/hosts/ds1ptde/gameplay.h), [core/native_action.h](../framework/core/native_action.h), [profiles.h](../framework/characters/mario64/profiles.h), [projectile.h](../framework/hosts/ds1ptde/projectile.h), [fire_flower.h](../framework/characters/mario64/fire_flower.h). The remapped controls need live checks of both hands, spell-use counts, shield guard/parry, held buttons and LT/L2; other pyromancies are not individually verified.

## Cannon and Bob-omb Buddy

A cannon/Buddy pair is placed near the central Firelink bonfire. Talk with A/Q to unlock, enter with A/Q, aim with either stick/WASD, fire with A/Space and leave with Y/Q. Release entry before firing. Opening takes about five seconds.

The runtime uses libsm64 cannon actions, including staged missing opening actions, and decodes cannon/Buddy visuals from the owned ROM. This is a bounded interaction, not the complete SM64 object scheduler or Buddy dialogue/AI. Unlock state is shared between local saves through mod configuration.

`[WorldObjects.FirelinkLauncher]` exposes placement `X`, `Y`, `Z`, `Yaw` and `Unlocked`. A fixed cannon eye and the guest aim angles drive both the camera and launch direction. Launching grants no extra protection or guaranteed safe landing.

Sources: [launcher.h](../framework/hosts/ds1ptde/launcher.h), [runtime_cannon.h](../framework/characters/mario64/runtime_cannon.h), [cannon_assets.h](../framework/characters/mario64/cannon_assets.h). Details: [cannon](../framework/docs/CANNON.md).

## Rendering, audio and diagnostics

- The PTDE host observes the real D3D9 device rather than creating a dummy device. Stock and DSfix scene/depth paths support distinct scene/UI sizes; Mario and diagnostics draw before prompts, subtitles and menus.
- Simulation stays at 30 Hz. Render-only mesh/attachment and native-anchor interpolation smooths higher frame rates, with about one tick of visual delay. Lifecycle changes, teleports and incompatible topology reset interpolation.
- Native enemy/boss audio remains a host responsibility; libsm64 supplies Mario audio through a separate bounded channel. Audio parity still needs area-specific listening checks.
- F6 toggles Mario/native. F8 toggles an initially hidden overlay; F9 cycles views; F10 toggles through-walls drawing. The overlay shows capsules, attack bounds, contact/HP confirmation, cached terrain and object origins, not a full native collision editor.
- `mariosouls.log` is fresh per launch and capped at 256 KiB. It contains startup/errors and limited state changes, without movement recording or session journals. Bug reports use the manual GitHub template.

Sources: [render_character.h](../framework/hosts/ds1ptde/render_character.h), [render_composite.h](../framework/hosts/ds1ptde/render_composite.h), [mesh_interpolation.h](../framework/core/mesh_interpolation.h), [debug_view.h](../framework/hosts/ds1ptde/debug_view.h).

## Framework, build and installation

The host/core/SDK/adapter split, host capabilities, renderer interface and portable types are retained for future characters and Remastered. Only PTDE/D3D9 and Mario are implemented. The host still has a single translation unit with internal feature headers; adapters use the C SDK.

Pinned dependencies/compiler hashes, local asset preparation, build version/revision/file hashes, licenses and production validation remain. CI is compilation only. A single `MarioSouls.ps1 install` / `uninstall` entry point owns reversible PTDE file operations, recognizes the supported DSfix chain and preserves existing settings/assets/saves.

The version-checked startup hook skips logo requests by default (`[MarioSouls] SkipIntro=1`). Normal title/character selection remains; there is no development autoload or save redirection in the current project.

The earlier experimental Link adapter, custom equipment-load penalty, cheat/developer menus, test lab, command bridges, replay, F11 reporting, Steam-free developer-copy tools and old packaging wrappers have been removed. F1..F4, F7 and F11 have no mod function. Git remains the source checkpoint system; existing tags and private archives are preserved.

See [architecture](../framework/README.md), [development](DEVELOPMENT.md), [installation](INSTALLING.md), [host porting](../framework/docs/HOST_PORTING.md) and [recovery](RECOVERY.md).
