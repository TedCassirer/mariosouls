# PTDE host implementation

This host connects the framework to the supported 32-bit Dark Souls: Prepare to Die Edition executable. It owns native state, hooks, D3D9, input, collision, menus, damage, equipment, class integration and edition-specific asset/setup rules. Mario is reached only through neutral SDK capabilities.

Follow the [architecture rules](../../docs/ARCHITECTURE.md) when extending this host. Shared policies must not read native memory; adapter headers/guest IDs must not leak here. Native readiness/observation is mapped in `capabilities.h`; common operation requirements supplement rather than replace signatures, ownership and local validation. D3D resources/pipeline drawing belong in `render_device.h`, while presentation modules submit neutral geometry/materials.

## Source entry points

| Responsibility | Files |
| --- | --- |
| Composition/startup/native lifetime | `runtime.cpp`, `startup.h`, `loader.h`, `native_bindings.h`, `graphics_bootstrap.h` |
| Frame observation and routing | `gameplay.h`, `action_input.h`, `controller.h`, `controller_hooks.h` |
| Lifecycle/presentation | `character_state.h`, `presentation.h`, `character_visibility.h`, `death.h`, `cutscene.h` |
| Native action ownership | `actions.h`, `interaction.h`, `native_reaction.h`, `equipment_actions.h` |
| Motion/support/terrain | `world.h`, `native_support.h`, `terrain.h`, `arena_floor.h`, `body.h`, `hazards.h` |
| Damage/equipment/reactions | `combat.h`, `native_combat.h`, `equipment_native.h`, `defense.h`, `recovery_native.h` |
| Rendering and UI order | `render_observation.h`, `render_character.h`, `render_device.h`, `render_surfaces.h`, `render_composite.h`, `ui_order.h` |
| Read-only diagnostics | `debug_observation.h`, `debug_view.h`, shared `core/debug_presentation.h` |
| Class/preview | `starting_class.h`, `class_preview.h`, `unarmed_equipment.h` |
| Props/assets/setup | `equipment_props.h`, `archive.py`, `prepare_collision.py`, `prepare_equipment.py`, `setup.py`, `player_setup.py`, `installer.cs`, `edition.json` |

The host builds as one translation unit. Feature headers are internal implementation fragments with shared state and an explicit include order; they are not public standalone modules. Keep a new feature at the correct ownership boundary rather than satisfying dependencies through arbitrary includes.

The separate update routine currently runs from `endScene` once per Present. Fixed-step/backpressure and draw submission are separated, but the native scheduling integration is still tied to graphics opportunities. Do not claim otherwise or move simulation into a drawing helper. See the architecture's current-debt section before reusing this arrangement in another host.

## Update and ownership flow

`runtime.cpp` is the composition root. `DllMain` stays small and existing startup work runs on its startup thread. Native binding validates executable/signatures and installs hooks in their defined order. The module definition preserves both DirectInput export spellings.

`gameplay.h` observes actor/camera/input, resolves lifecycle intent, routes contextual actions, reconciles native movement/support and advances a ready fixed-rate guest tick. Native-owned action/cinematic/death presentation is separate. `render_character.h` consumes validated snapshots; drawing cannot call guest simulation.

`player`, `playerPhysics` and `playerCamera` describe current observations. `characterSession.native.actor` and `.movement` identify the objects whose scoped modifications may be restored. `loadedMap` identifies terrain; `characterSession.instance.origin` converts native metres into local SDK centimetres. `characterSession.control` owns intent, session and clock/input latches. Resetting simulation does not invent a new class choice or ownership epoch.

Native controls retain menus, scripts, paired combat and death. Guest appearance can persist while locomotion waits for a safe native identity. Visibility suppression begins only after usable replacement data/drawing exists; invalid native identity or teardown restores scoped fields safely.

## Native gameplay boundaries

- HP/stamina, effects, inventory, quests, damage/defenses and rewards remain native.
- Motion tickets carry generation/sequence identity. Native physics resolves displacement; support/contact patches are temporary and refresh on rebasing.
- Event-controlled doors/floors/props remain native-owned. Cache preparation must not recreate a destroyed/disabled barrier. The dynamic Asylum floor is gameplay integration, not a test fixture to delete.
- Enemy targets are generation-bearing native handles. Validate resolution before contacts and HP confirmation, especially after streaming.
- Weapon packets retain only ownership/validation fields. Guest attack packets have independent pre-defense power and cannot inherit unrelated equipment effects.
- Stamina/reaction/fire hooks preserve their exact native conventions, including floating-point state. Never remove a hook because an old callback name resembles telemetry.

## Graphics and DSfix

The renderer observes the game's actual D3D9 factory/device. It recognizes scene/depth and before-HUD boundaries without dummy-device creation or private DSfix layouts. Scene and UI sizes can differ. Device/resource restoration and UI priority are host responsibilities; the neutral renderer interface does not expose COM handles.

Stock/DSfix have focused historical checks. A native-hook compile or successful draw submission is not full visual/audio compatibility evidence. Cutscene replacement currently uses a neutral guest pose under native cinematic transforms, with fallback on ambiguous data.

## Edition preparation and future ports

`edition.json` owns executable identity/default paths; `setup.py` performs reversible install/uninstall behind root `MarioSouls.ps1`. Collision/equipment recipes read the owner's PTDE files. These rules must not leak into another edition or into guest adapters.

Remastered requires new native discovery and a DX11 host implementation. Preserve the [integration inventory](PORT_CHECKLIST.md), [host-porting obligations](../../docs/HOST_PORTING.md), [SDK contract](../../docs/CONTRACT.md) and [manual checklist](../../../docs/TESTING.md). Update [Features and tweaks](../../../docs/FEATURES_AND_TWEAKS.md) when behavior/defaults change.

Logging is bounded startup/error information. F8 diagnostics are read-only observations; no gameplay automation or cheat menu is part of this host.

## Explicit presentation and lifetime inputs

`render_observation.h` captures/validates actor/camera/preview/cinematic data and supplies the neutral camera/mesh builders in `core/character_presentation.h`. `render_character.h` composes the result; `render_device.h::StateScope` owns synchronous D3D9 state restoration. The original update/hook cadence remains intact.

`debug_observation.h` owns native/guest capture and retention; `core/debug_presentation.h` owns value-only diagnostic shape/panel assembly; `debug_view.h` submits it through the renderer. Native addresses and probing remain in this edition.

`character_state.h` explicitly owns module/CPU mesh resources and the non-copyable instance/control/visual/native-lease/presentation session. CPU storage is neutral `core/character_storage.h`. Cleanup order is still explicit in `runtime.cpp`, with current-actor checks before restoring native bits. Preview/cinematic/audio and feature-local statics/include ordering remain bounded debt. See the [port handoff](../../docs/REMASTERED_HANDOFF.md); these headers are not a portable native host implementation.
