# Mario 64 adapter

This adapter runs the pinned libsm64 guest engine and translates its state into
the Souls Character SDK. Movement, action identification, animations and Mario's
props belong here. Dark Souls owns health, stamina, equipment damage, inventory,
quests and rewards through the host.

## Resource ownership

`adapter.cpp` exposes the SDK callbacks and separates three lifetimes:

- `MarioRuntimeLibrary` holds the loaded DLL and typed exports. All required
  exports are resolved before the guest engine is initialized.
- `AdapterResources` retains the owned ROM and initialization state until SDK
  shutdown. ROM data must remain alive while libsm64 is using it.
- `MarioInstance` holds the single live guest handle, engine state, action history,
  equipment sockets and appearance. Destroying and recreating the character does
  not unload the runtime or ROM.

Initialization failures roll back through shutdown and permit a later retry.
The host must call SDK shutdown before unloading the adapter. The runtime has
global guest state, so this adapter permits one live Mario instance at a time.
Repeated class-preview creation uses the same create/destroy contract.

`writeFrameworkState()` converts engine units and guest flags into SDK values.
The private identifiers in `actions.h` document the pinned guest action metadata;
the host does not interpret those IDs. A guest action's invulnerability flag is
used for action classification and never grants native Dark Souls protection.

## Source map

| Files | Responsibility |
| --- | --- |
| `adapter.cpp`, `character.json` | SDK callbacks, state translation and character tuning |
| `runtime_library.h`, `actions.h` | Typed guest exports and private action identifiers |
| `runtime_body.c`, `runtime_*.h` | Adapter-owned runtime extensions for body, motion, equipment and cannon behavior |
| `cannon_assets.h` | Bounded decoding of cannon/Buddy data from the owned ROM |
| `fire_flower.h` | Procedural flower and Fire Mario presentation |
| `build_runtime.py`, `prepare_runtime.py`, `runtime_geometry.py` | Build the staged pinned runtime without editing the original checkout |
| `author_equipment.py`, `author_flower.py`, `*_keys.h` | Offline pose fitting and authored joint keys |
| `*_test.cpp` | Character-specific offline regression fixtures |
| `*_preview.cpp` | Private pose dumps used by the equipment preview workflow |

The authoring tools are maintained asset recipes, not gameplay dependencies.
See [Equipment](EQUIPMENT.md) for pose fitting and supported weapon families.

## Build and verification

Use the repository's [development workflow](../../../docs/DEVELOPMENT.md).
`python tools/framework.py build --with-runtime` builds the host, adapter and
runtime. The full test command requires a user-supplied US big-endian Mario 64
ROM; no ROM is downloaded or included in source control.

The runtime build recipe separates command construction from execution. Importing
it or asking for help does not stage sources, check dependencies or start a
compiler. An actual build stages a private source copy, applies the pinned
extensions there and propagates compiler failures to the framework command.

The full suite covers actual guest movement and mesh output, synthetic collision,
traction, death presentation, equipment, runtime failure recovery and repeated
initialization/shutdown. Native gameplay remains a separate verification layer;
see [Validation](../../docs/VALIDATION.md). Building or testing does not install
the mod or touch player saves.
