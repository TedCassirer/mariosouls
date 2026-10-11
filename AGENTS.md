# Character framework development

All new gameplay integration work belongs to `framework/`.
- Put Dark Souls addresses, hooks, collision, menus, health, stamina and native damage in `framework/hosts/ds1ptde/`.
- Put character-independent policies, lifecycle checks and validation in `framework/core/`.
- Put guest engine APIs, action/animation IDs, movement rules and character meshes/props in `framework/characters/<id>/`.
- Share changes through the SDK rather than adding character switches to the host. Character-specific fixes belong in that character's adapter.
- `work/` contains the preserved 0.7 prototype and investigation history. Do not continue implementing gameplay there. Compatibility build entry points may delegate to the framework.
- Use `python tools/framework.py build`, `test`, `doctor`, and `new-character` as the supported workflow. Keep the SDK contract and adapter guide current when its capabilities change.
- Never claim a stub is a functioning port. Link/OoT and DK/DK64 require separate implementations and user-supplied assets; they are not implemented here.
- Keep ROMs, saves, game assets, generated collision, binaries and private test captures out of Git. Keep existing checkpoint tags and local archives immutable.
- Native Dark Souls HP, stamina, inventory, quest state and rewards are authoritative. No character adapter may write game memory or grant implicit jump invulnerability.
- Preserve the test-character guard; never test on the user's level-65 character. Existing controller permission is subject to the user's current control handoff.
