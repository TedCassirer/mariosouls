# Mario adapter

This adapter runs the pinned libsm64 guest runtime inside the character framework. It owns SM64 actions, movement translation, ROM decoding, animation, meshes and props. The PTDE host owns native collision resolution, HP/stamina, inventory, damage, quests and rewards.

Mario is the only implemented character. The adapter demonstrates reusable SDK capabilities; it is not an engine for OoT, Wind Waker or DK64 characters.

Follow [ARCHITECTURE.md](../../docs/ARCHITECTURE.md): this adapter may depend on neutral core/SDK and its own guest runtime/assets, but never native Dark Souls memory/hooks/IDs or graphics drawing. Keep host observations semantic at the SDK boundary and guest action/animation IDs here. Loading this adapter's own runtime DLL with Windows APIs does not grant permission to inspect the game process.

## Resource and instance lifetime

`adapter.cpp` separates three lifetimes:

| Type | Lifetime |
| --- | --- |
| `MarioRuntimeLibrary` | Loaded runtime DLL and typed required/optional exports |
| `AdapterResources` | Initialization, owned ROM storage, logging and decoded cannon resources |
| `MarioInstance` | Current handle, engine state, action history, equipment sockets and fire/burn presentation |

All required runtime exports are resolved before initialization. Initialization failure rolls back through shutdown; a later create/destroy cycle must work without unloading the ROM/runtime. The engine permits one live Mario at a time. The host coordinates gameplay/preview/presentation lifetimes accordingly.

`writeFrameworkState` translates guest units and raw action metadata into semantic SDK state. Raw action IDs remain diagnostic to the host. Guest invulnerability flags classify actions but never grant native protection. Independent guest health/death is suppressed under the native-health policy.

## Source map

| Files | Role |
| --- | --- |
| `adapter.cpp`, `actions.h` | SDK callbacks, guest action mapping and contact/motion translation |
| `character.json`, `profiles.h` | Descriptor/defaults and optional class, glove, camera, impact, projectile, defense/recovery profiles |
| `runtime_library.h` | Typed guest exports and loading contract |
| `poses.h` | Native semantic pose to guest clip/style/phase |
| `runtime_body.c`, `runtime_*.h`, `prepare_runtime.py` | Adapter-owned staged runtime extensions |
| `item_props.h`, `fire_flower.h` | Authored use-item/flower geometry and Fire Mario presentation |
| `cannon_assets.h` | Bounded owned-ROM cannon/Buddy decoding |
| `equipment_keys.h`, `flower_keys.h`, `author_*.py` | Authored arm keys and fitting tools |
| `*_preview.cpp` | Private pose dumps for manual equipment review |
| `build_runtime.py`, `runtime_geometry.py` | Pinned runtime build and staged geometry preparation |

Optional profile/item-prop headers are internal implementation fragments. Guest-specific fixes belong here instead of native host switches for Mario. Shared neutral behavior belongs in core/SDK.

## Gameplay mapping

The body profile separates a 0.90 model scale, smaller movement/contact capsules and heavily reduced ordinary landing damage from movement speed/jump distance. Default terrain receives added grip without changing designated slippery surfaces. Native correction preserves tangential momentum and current action phase.

Mario's Gloves gate damaging unarmed moves and use saved normal Caestus +0..15 reinforcement. Equipped weapon attacks use their own native damage path. Class preview, interactions, equipment poses, death, environment reactions and launcher state are semantic SDK integrations rather than native-memory operations in the adapter.

Important default values, configuration keys and verification limits live in the maintained [feature inventory](../../../docs/FEATURES_AND_TWEAKS.md). Equipment fitting is described in [EQUIPMENT.md](EQUIPMENT.md); fire/lava and cannon behavior have [environment](../../docs/ENVIRONMENT_REACTIONS.md) and [launcher](../../docs/CANNON.md) guides.

## Build and review

```powershell
python tools/framework.py build --with-runtime
python tools/framework.py build --check-portability
```

Runtime preparation copies pinned sources into private build staging, applies the adapter extensions there and propagates compiler failure. Importing the recipe or requesting help does not stage/build. The original dependency checkout is not modified by a runtime build.

Playing requires the owned US SM64 ROM; source/binary compilation does not exercise it. Generated geometry, pose dumps and previews remain private. x64 guest compilation prepares the architecture boundary but does not implement Remastered. Use [manual verification](../../../docs/TESTING.md) and update the behavior catalog/changelog for changes.
