# Souls Character Framework

The framework connects character engines to Dark Souls: Prepare to Die Edition.
The host owns the game's state and native integration; each adapter owns its
character's movement, animations and presentation.

MarioSouls is the primary adapter. Wind Waker Link has an experimental playable
adapter with an authored controller and user-supplied assets. It does not embed
the Wind Waker engine. Ocarina of Time Link and DK64 are not implemented.

## Architecture

| Directory | Responsibility |
| --- | --- |
| `hosts/ds1ptde/` | Native hooks, rendering, collision, input, menus and damage |
| `core/` | Shared lifecycle, movement ownership, combat and validation policies |
| `characters/<id>/` | Guest engine APIs, character rules, animation and props |
| `sdk/` | Versioned interface between the host and adapters |
| `templates/` | Starting point for a new adapter |
| `tests/` | Offline policy, rendering and adapter checks |
| `qa/` | Guarded live checks using dedicated test characters |

Dark Souls HP, stamina, inventory, quests and rewards are authoritative. Adapters
do not write game memory. Movement is reconciled with native physics; jumping
does not grant implicit invulnerability. See the [SDK contract](docs/CONTRACT.md)
and the repository's [development rules](../AGENTS.md).

## Build and validate

Use the supported `tools/framework.py` workflow from the repository root:

```powershell
python tools/framework.py doctor
python tools/framework.py build --with-runtime
python tools/framework.py test --asset "C:\path\Super Mario 64 (USA).z64" --runtime "work/framework-build/characters/mario64/sm64.dll"
```

See [development setup](../docs/DEVELOPMENT.md) for prerequisites, local asset
preparation and installation. Build output goes to `work/framework-build/`.
Building does not install anything. Offline checks do not replace live gameplay
checks, and installation requires the exact tested build.

## Documentation

| Topic | Guide |
| --- | --- |
| Adapter interface and ownership | [SDK contract](docs/CONTRACT.md) |
| Creating a character | [Adapter guide](docs/ADDING_A_CHARACTER.md) |
| Mario adapter internals | [Mario implementation](characters/mario64/README.md) |
| Mario weapons and animation fitting | [Equipment](characters/mario64/EQUIPMENT.md) |
| Wind Waker Link's scope and setup | [Link adapter](characters/link_ww/README.md) |
| Starting classes and previews | [Class integration](docs/STARTING_CLASSES.md) |
| Collision and combat diagnostics | [Debug overlay](docs/DEBUG_VIEW.md) |
| Hit feedback | [Feedback](docs/FEEDBACK.md) |
| Kick knockback | [Kick impacts](docs/KICK_IMPACTS.md) |
| Cannon behavior and limitations | [Cannon](docs/CANNON.md) |
| Actual test coverage | [Validation record](docs/VALIDATION.md) |

## Configuration

The installed host retains `mariosouls.ini` and `mariosouls.log` for compatibility.
Adapters are selected at startup through `[Framework] Character`; live adapter
swapping is not implemented. F6 toggles the selected character's mode.

Independent unarmed power is configured under `[Combat.<id>]`. Supported equipped
weapons use native weapon power and scaling. Mario upgrade levels are currently
configuration values; an inventory/titanite upgrade interface is not implemented.

Mario's wider camera uses `[Camera.mario64] DistanceScale` (default `1.5`).
`[Projectiles] Enabled=0` disables the bouncing-fireball extension. The feature
guides above document further settings and their limits.
