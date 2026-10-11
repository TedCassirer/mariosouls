# Dark Souls PTDE host

This directory owns the native game boundary: memory, hooks, input routing,
collision, rendering, menus, inventory, damage and lifecycle integration.
Character adapters request behavior through the SDK; they do not read or write
Dark Souls memory.

`runtime.cpp` is the DLL composition root. It binds the game, loads the adapter,
coordinates ownership changes, advances gameplay and installs Direct3D hooks.
Many internal feature headers currently share its file-local state and must be
included in dependency order. They are implementation fragments, not standalone
SDK headers. The independent policy headers under `framework/core/` are tested
separately.

Useful starting points:

| Area | Files |
| --- | --- |
| Native boundary | `memory.h`, `loader.h`, `native_targets.h` |
| Ownership and presentation | `presentation.h`, `character_visibility.h`, `death.h` |
| Movement and collision | `world.h`, `body.h`, `arena_floor.h`, `hazards.h` |
| Input and interactions | `controller.h`, `action_input.h`, `interaction.h`, `actions.h` |
| Combat and equipment | `combat.h`, `native_combat.h`, `equipment.h`, `equipment_actions.h` |
| Rendering and UI order | `render_surfaces.h`, `render_composite.h`, `ui_order.h`, `ui_render.h` |
| Diagnostics and live QA | `diagnostics.h`, `report_files.h`, `debug_view.h`, `harness.h` |
| Character creation | `starting_class.h`, `class_preview.h` |

Native HP, stamina, inventory, quest state and rewards remain authoritative.
Actor pointers and cached geometry must be invalidated when their owning game
state changes. See the [SDK contract](../../docs/CONTRACT.md) and
[test guide](../../../docs/TESTING.md) before changing these boundaries.
