# Playing MarioSouls

## Release status

MarioSouls is experimental. There is no standalone public installer yet, and
cloning this repository does not give you a playable installation. Development
scripts currently depend on local tools, generated assets and configuration.

The intended release will support **Dark Souls: Prepare to Die Edition on
Windows**, with a user-supplied **US Super Mario 64 ROM**. Dark Souls Remastered
is not supported. No ROM or game assets will be provided with the mod.

## Basic controls

These are the default Mario controls in the current development build. Native
menus retain their own controls, and existing configuration can override bindings.

| Action | Controller | Keyboard/mouse |
| --- | --- | --- |
| Move | Left stick | WASD |
| Camera | Right stick | Mouse |
| Jump / interact at an eligible prompt | A | Space to jump; Q to interact |
| Alternate interaction | Y | Q |
| Attack / cast from the right hand | B or RB | Left mouse |
| Shield guard | LB | Right mouse |
| Parry / cast from the left hand | RT | Tab |
| Crouch | LT | Left Ctrl |
| Use selected item | X | E |
| Toggle character mode | Hold both stick buttons | F6 |
| Review and report a bug | Keyboard shortcut | F11 |

An empty right hand uses Mario's moveset. Supported equipped melee weapons use
native weapon damage and fitted swing animations. A selected Pyromancy Flame
enables Fire Mario's appearance; Fireball still needs to be attuned and consumes
native spell uses.

## Known limits

See [bug reporting](BUG_REPORTING.md) for F11, privacy details and diagnostic history.

- Collision, area transitions and scripted events need more full-game testing.
- Weapon support does not cover every family, two-handing or every attack type.
- Configurable Mario upgrades have no in-game upgrade interface yet.
- The experimental Wind Waker Link adapter has a separate, smaller feature set.

Use a separate test character and back up your saves. Consult the
[validation record](../framework/docs/VALIDATION.md) for measured coverage rather
than assuming a feature works in every area.

For diagnosis, F8 toggles the [debug overlay](../framework/docs/DEBUG_VIEW.md),
F9 changes its view and F10 toggles drawing through walls.
