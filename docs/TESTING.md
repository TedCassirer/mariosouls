# Verification

## What automated checks establish

```powershell
python tools/framework.py build --check-portability
```

This command compiles/links the PTDE host, Mario adapter and runtime for x86; compiles shared headers and C/C++ SDK declarations for x86/x64; and builds Mario/runtime for x64. Static assertions cover pointer sizes, structure sizes/offsets and calling conventions. No game, adapter or check program is executed.

CI prepares pinned dependencies, runs this build and keeps build logs for seven days. There are no automated behavioral, ROM, rendering, Python, formatting or documentation test suites. Production executable, pointer, hook and asset checks remain active in the mod.

## Manual game checks

Use disposable characters and separate save backups. Never test on the owner's level-65 character. Agent-driven game input needs a current handoff. The project has no automated movement bridge, cheat menu, save editor, scenario runner or special game launcher.

| Area | Check |
| --- | --- |
| Creation | Mario class name/model; trait submenus; tutorial glove/Flame progression |
| Load / reload | Saved Mario class restores the model; unrelated classes do not inherit stale intent |
| Toggle | F6 / both sticks restore native behavior and return to Mario predictably |
| Movement | Run/jump, slopes, stairs, narrow openings, ledges and unsupported descent |
| Dynamic support | Ride a lift both ways, jump off/on, remove an event floor and cross a coordinate rebase |
| Overlay | Hidden at startup; F8, F9 and F10 behave without changing gameplay |
| Interactions | Contextual A, NPC, door, bonfire, ladder and Estus; include slopes and open menus; Y must not trigger world interaction |
| Combat | Glove +0/+upgrades, each attack, stamina exhaustion, weapon families, blocks, parries and criticals |
| Fire Flower controls | Left-hand LB/L1/right mouse cast versus right-hand B/RB/left mouse; native charges; RT/Tab must not cast; shield LB guard/RT parry; LT/L2 crouch/ground pound; release/repress after casting, item/focus changes and menus; left Flame with empty right hand/gloves/sword, flat ground/stairs; 150-ms native footing preparation or one-second cancellation with no queued input; compare `[Projectiles] Enabled=0`; F6 restores native bindings |
| Incoming damage | Melee/projectile hits, armor/statuses, healing, heavy/airborne recovery, lethal damage and grabs |
| Environment | Reduced landings versus lethal pits; lava with/without ring; enemy-fire priority/cooldown |
| Death / streaming | One-shot death, respawn, and area crossing while moving followed by successful attacks |
| Rendering / audio | Stock/DSfix, scene/UI resolutions, 30/60 FPS, UI above Mario, reset/focus and native enemy/boss audio |
| Cutscenes / cannon | Player transform/camera/fallback; unlock, aim in multiple headings, launch, land and cancel |

This is a checklist, not a statement that every case already passes. Scope each check to the changed behavior rather than claiming broad compatibility from a small sample.

After renderer/capability/session refactoring, repeat the relevant rows in stock PTDE and DSfix: character and held-prop textures/lighting, impact flashes, cannon/reticle/prompt, all F8 layers/depth modes, native UI ordering, resolution change and device reset. Include creation-preview trait menus, cinematic camera/root handling, loading/movement, interaction/Estus, combat, death/respawn and a moving area transition. The current source extraction also separates native render/debug observation from CPU assembly and groups native restoration/guest state; explicitly verify F6 restoration and device reset do not cross those lifetimes. Compilation does not establish visual or fallback parity; no such live check was performed for this architectural change. No Remastered game check is possible from the PTDE build.

## Record useful evidence

Record the commit/build, game fingerprint, DSfix/FPS/resolution settings, input device, disposable character, area, steps, observed native HP/stamina/animation behavior and remaining gaps. A screenshot of an animation is not proof of damage/protection; a successful API call is not proof of audible sound or physical rumble.

Keep private captures and owned data outside Git. Summarize evidence in the PR and update verification limits in [Features and tweaks](FEATURES_AND_TWEAKS.md). Historical checks and compilation should be labeled separately; [compatibility](COMPATIBILITY.md) describes the current evidence scope.
