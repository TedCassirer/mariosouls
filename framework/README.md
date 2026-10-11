# Souls Character Framework 0.8.2

Mario's Fire Flower now launches a fast, bouncing Fireball. Casting takes about
0.9 seconds instead of the measured native 2.7 seconds. Floor contacts bounce the
same native projectile; walls and enemies retain native impacts, damage and
rewards. Each shot still consumes one attuned Fireball use. This applies to the
ordinary Fireball spell with a selected Pyromancy Flame in either hand.
The optional SDK projectile profile separates character tuning from host hooks.
Set `[Projectiles] Enabled=0` in mariosouls.ini to disable this extension.
See [validation](docs/VALIDATION.md) for measured coverage and limitations.

Mario's camera is now 50% farther away while his mode is active. The shared host
preserves native camera collision and restores native distance on handback.
Override `DistanceScale=1.5` under `[Camera.mario64]` in mariosouls.ini; 1 restores
the original distance. The optional camera-profile export keeps ABI4 compatible
and lets future adapters opt in without host character switches.

Fire Flower follow-up: selecting a Pyromancy Flame in either hand gives Mario
a procedural flower, white hat/shirt and red overalls. With Fireball attuned,
B/RB/left-click casts from the right hand; RT/Tab casts from the left. The native
game owns spell charges, hit detection, damage and rewards; the optional bouncing
profile extends floor-contact behavior. The optional
SDK pyromancy capability keeps host casting separate from adapter props/poses.
Fireball is verified on Mario QA; other spells need additional coverage.

Native combat follow-up: adapters can opt into native parries, backstabs and
ripostes while supplying their own visible poses. Mario uses RT/Tab to parry
with a supported shield and the normal attack button for a nearby critical
opportunity. Native eligibility, stamina, damage and paired enemy animation
remain authoritative. See the validation record for measured coverage.

Animation follow-up: six weapon families now have individually fitted windups,
strikes and recoveries, with torso movement and planted feet. The SDK-based
`preview-equipment` workflow renders front/rear visual checks without launching
the game. See the Mario equipment guide for the private asset requirements.

Death presentation follow-up: Mario now plays his original SM64 collapse and
death voice once, then holds the final pose while Dark Souls handles "YOU DIED"
and respawning. This uses the optional SDK death-pose capability and shared
session lifecycle. It does not add a second health pool or guest respawn logic.

Interaction follow-up: controller A now interacts at eligible native prompts
and jumps elsewhere. Y remains an interaction alias. A weapon-stance idle check
that rejected animation 120000 has been fixed. The user confirmed A recovered
Mario QA's 212 souls without jumping; see the validation record for scope.

0.6.0 expands the optional ABI4 equipment path to one-handed straight swords,
daggers, curved swords, greatswords, axes and hammers/maces. Mario can carry these
with a small or medium shield while retaining his own
locomotion and fitted arm animations. Ground sword swings use the selected
weapon's native power, reinforcement, stat scaling and attack stamina cost.
An empty right hand retains Mario's original moves and independent attack power.
The host owns damage and guard rules; adapters own their arm poses and sockets.

This equipment implementation covers two grounded light attacks per family
and held blocking, plus the optional native parry/critical path. Two-handing and
the remaining weapon families are not implemented. Temporary weapon-buff parity is not verified. Props are suppressed during hurt
reactions and native interactions. Wind Waker Link does not yet opt into equipment
fitting. See [verification](docs/VALIDATION.md) for measured in-game coverage.

Wind Waker Link is the second real adapter, using an authored controller and
privately extracted original assets. Shared skeletal asset loading, offline
preparation, cutout materials and kinematic queries support future adapters.
See [Link's scope, preparation and limitations](characters/link_ww/README.md).
Link/OoT and DK/DK64 remain separate, unimplemented projects.

MarioSouls is the first character adapter for a shared Dark Souls PTDE integration.
This is a working extraction of the installed 0.7 integration, with a versioned
DLL interface, host-independent combat policy, manifest-driven attacks, a character
generator, conformance checks and a backed-up installer. It is not a universal
importer for arbitrary game models or ROMs.

| Directory | Responsibility |
| --- | --- |
| `sdk/` | Versioned C interface between the host and each character |
| `core/` | Movement ownership, lifecycle, input buffering, combat, feedback and validation |
| `hosts/ds1ptde/` | Native collision, rendering before UI, input, interactions, health, stamina and damage |
| `characters/mario64/` | Mario's engine, action IDs, animations, sounds, flask and equipment fitting |
| `templates/` | Compilable starting adapter for a future character |
| `tests/` | Shared policy and real DLL conformance tests |
| `qa/` | Guarded observation and controls for Mario QA: low-level or exact level-13 melee fixture, Asylum only |

Build from the repository root with the pinned dependencies installed:

```powershell
python tools/framework.py doctor
python tools/framework.py build --with-runtime
python tools/framework.py test --asset "C:\path\Super Mario 64 (USA).z64" --runtime "C:\absolute\project\work\framework-build\characters\mario64\sm64.dll"
python tools/framework.py install --asset "C:\path\Super Mario 64 (USA).z64" --runtime "C:\absolute\project\work\framework-build\characters\mario64\sm64.dll" --check-only
```

Close Dark Souls normally, then repeat `install` without `--check-only`. Builds
alone do not install anything. The installer requires tests for the exact host,
adapter, runtime and assets, checks the game executable and backs up every replaced
file. On a failed copy it restores those files. ROMs are referenced, never copied.

`build` without `--with-runtime` rebuilds the host and adapter only. `--adapter-only`
is useful for adapter development but intentionally cannot produce an installable
receipt: make a full build and run tests before installing. Build products and
private QA evidence go under `work/framework-build/`.

To prepare a map's collision from the installed game (private generated output):

```powershell
python tools/framework.py prepare-collision --map m18_01_00_00 --output work/my-new-collision-cache
python tools/framework.py install --character mario64 --collision-directory work/my-new-collision-cache
```

The installer still requires an exact tested build and the configured owned assets.
It verifies and backs up the cache with the runtime. Unprepared maps fall back to
the existing legacy caches; only the Asylum was regenerated and checked in 0.4.1.

Prepare the equipment models from the installed game into a new private directory:

```powershell
python tools/framework.py prepare-equipment --output work/my-new-equipment-cache
python tools/framework.py install --character mario64 --equipment-directory work/my-new-equipment-cache
```

Supply the usual asset/runtime arguments when they are not already configured.
Preparation reads native parts and textures, producing 48 melee weapon models across six families
and 34 small/medium-shield models. These are local game assets, never repository
content. The installer checks their manifest, hashes, geometry and supported game
executable, then installs them under `DATA/SoulsCharacterFramework/equipment/`
within the normal backup/rollback transaction. Later installs preserve the
configured `EquipmentDirectory`. Preparing all models does not establish live
combat coverage for every weapon, reinforcement path or shield.

With supported equipment selected in the native inventory:

| Action | Controller | Keyboard/mouse |
| --- | --- | --- |
| Attack | B or RB | Left mouse |
| Hold shield guard | LB | Right mouse |
| Jump when no interaction prompt | A | Space |
| Crouch | LT | Left Ctrl |
| Interact | A when prompted; Y alias | Q |
| Use selected item | X | E |

Sword swings commit Mario briefly; holding attack does not repeat swings.
Pressing again near the end queues the next swing. Insufficient stamina prevents
the swing. Guarding slows ground movement and uses native shield defenses and
stamina; jumping, crouching, attacking or interacting releases it.

Installed adapters live under `DATA/SoulsCharacterFramework/characters/<id>/`.
`[Framework] Character` selects one adapter on startup; live swapping is not
supported. The existing `mariosouls.ini`, `mariosouls.log` and `MarioSouls/collision`
names are retained for compatibility with the user's controls and private caches.
The host does not interpret Mario engine actions. Those names are storage
compatibility, not separate implementations.

Independent guest-attack balance overrides use `[Combat.<id>]`, including
`BasePower`, `UpgradePower`, `UpgradeLevel`, `FixedPower`, `StatScaling`,
`<attack>_multiplier` and `<attack>_cost`.
Installation migrates the existing Mario `[Combat]` values once. Another character
starts with its manifest defaults. Upgrades remain configurable test values;
this refactor does not add an inventory/titanite upgrade shop. Equipped supported melee
weapons use their native weapon data instead of those independent power settings.

Version 0.2 introduces a single-use native movement mailbox and explicit ownership
for character locomotion, native actions and loading/death recovery. Each guest
step waits for its previous request to be consumed; the native capsule position
is sampled before collision correction and combat. Position corrections preserve
animations and clip only momentum directed into confirmed native contact planes.
Queued input and movement cannot survive an ownership change. ABI 2 makes this
contract available to every adapter through `resolveMotion`; ABI 1 DLLs must be
rebuilt. See [movement ownership](docs/CONTRACT.md#movement-ownership-abi-2).

Shared impact feedback now adds a short attacker pause, procedural contact sound,
depth-tested contact burst and controller vibration after native target HP falls.
Attack manifest multipliers determine the weight of the response, so other
characters inherit it without host branches or SDK changes. Misses and contacts
that do not lower HP do not trigger this damage feedback. Native guard reactions
are left to the host. See [impact feedback](docs/FEEDBACK.md) for settings and limits.

See [the ABI contract](docs/CONTRACT.md), [adding a character](docs/ADDING_A_CHARACTER.md)
and [verification](docs/VALIDATION.md). New gameplay work follows the root
`AGENTS.md`. The old `work/` implementation is retained as history; its two build
entry points now delegate to this framework. The old checkpoint tag and runtime
archive are untouched.


Version 0.3 adds independent model/body sizing and a landing damage policy to the
shared descriptor (ABI 3). Mario's body profile reduces collision dimensions and
landing damage without changing his movement speed or jump height. See the
contract and validation report for exact dimensions and measured coverage.

Version 0.4.1 fixed large-enemy contact by querying the native capsule surface and
removed event-controlled colliders from guest static caches. Native Havok still
owns those barriers. Mario's reduced body and fall policy are retained.
