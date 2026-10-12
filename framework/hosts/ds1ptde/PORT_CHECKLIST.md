# PTDE integration inventory for a future host

These are implemented **PTDE** boundaries, not known Remastered hooks. Each row
states what must be re-established on an owned future installation. File names
refer to this directory. Native structures and IDs must stay here. Verification
listed below describes existing methods; offline checks do not prove live game
compatibility. A new host must supply its own evidence.

| Boundary / files | Purpose | Inputs | Outputs | Verification method |
| --- | --- | --- | --- | --- |
| `runtime.cpp`, `startup.h`, `edition.json`, `proxy.def` | Gate native startup, skip logos and load the proxy | PE marker, startup signature, executable hash at install, loader exports, config | Accepted x86 process or inactive host; scoped logo skip; original system exports | Installer hash rejection; loader build; guarded normal launch (live check still required) |
| `memory.h`, `native_bindings.h` | Safe reads and unique signature discovery | Executable sections, signatures, transient pointer chains | Player/camera/map/event slots, validated reads | `native_bindings_test`; bad/ambiguous signatures; live pointer observations |
| `runtime.cpp`, `character_state.h`, `loader.h` | Compose host, validate/load adapter, sequence frame phases | Host observation, ABI table, settings, elapsed time | Owned buffers, lifecycle changes, guest callbacks | ABI/policy suites, adapter runner; live load/reload |
| `presentation.h`, `character_visibility.h` | Keep guest visible while native controls actions | Saved class intent, native actor identity, guest mesh | Guest pose and scoped native visibility suppression/restoration | `presentation_test`, `class_test`; live native actions and area changes |
| `death.h`, `native_reaction.h` | Map native HP/death/hurt to guest presentation | Authoritative HP, native animation and actor | Hurt/death pose, terminal hold, respawn cleanup | Guest death suite, simulated death/respawn; live lethal damage and reload |
| `world.h` | Submit motion at native physics boundary and reconcile it | Metre-space ticket, identity/generation, native contact planes | Authoritative resolved position, local-cm SDK resolution, dynamic support | Motion/contact suites; live doorways, slopes, elevators and removed floors |
| `arena_floor.h`, `prepare_collision.py`, `archive.py` | Extract and select edition-owned static collision | Map archives and executable fingerprint | Bounded cache, loaded terrain and native event ownership | Collision extractor fixtures, cache manifest checks; live map streaming |
| `body.h` | Scope native movement and contact capsule changes | SDK body policy, native body pointers and original dimensions | Reduced capsule and reliable restoration | `body_test`; live narrow doors, enemy hits and mode-off restoration |
| `hazards.h` | Preserve kill planes/pits independently of landing reduction | Native hazard observations and swept movement | Native lethal damage/death | Core hazard tests, hazard-host tests; live pit/landing damage separation |
| `camera.h`, `aim_camera.h` | Widen camera and aim around correct pivot | Native camera fields, guest camera/launcher profile | Scoped native camera values and restoration | `aim_camera_test`; live collision, aim directions and mode changes |
| `controller.h`, `controller_hooks.h`, `action_input.h` | Separate raw guest input and filtered native routes | XInput packets, focus, semantic action | Controller samples, contextual native confirm, packet sequence | Controller/action-input/hooks suites; live controller handoff |
| `interaction.h`, `actions.h` | Route native talk/use/rest/climb with guest presentation | Native menu/event/action observations, contextual confirm | Native interaction, guest pose, temporary native ownership | Context-action/motion suites; live bonfire, bloodstain, doors, ladders and NPC |
| `native_combat.h`, `param_templates.h`, `combat.h` | Apply guest attacks through native defenses | SDK attack, native stats/stamina, verified packet templates and target | Native damage, poise, death and rewards | Core/contact suites; live ordinary/armored/boss HP and stamina measurements |
| `native_targets.h`, `target_geometry.h`, `object_combat.h` | Resolve targets, body contact and destructible objects | Actor manager, native capsules, scene owners | Valid target identities and native object damage | Target/contact tests; live boss and destructible checks |
| `equipment_native.h`, `equipment_family.h`, `equipment.h`, `equipment_actions.h` | Translate native inventory/action IDs into neutral equipment semantics | Native equipped IDs, animation and stamina observations | SDK equipment pose, native guard/parry/critical handling | Equipment/native-block tests; guarded sword/axe/hammer/shield live checks |
| `equipment_props.h`, `prepare_equipment.py` | Extract/render native held props at guest sockets | Edition archives, neutral attachment matrices | Private bounded prop meshes | Equipment asset tests and offline previews; live socket alignment |
| `projectile.h` | Native projectile ownership and guest fireball tuning | Native projectile records, guest profile | Native damage projectile, scoped rate and trajectory | Core projectile suite; live bounce, enemy hits and restore |
| `effects.h`, `feedback.h`, `feedback_rumble.h` | Feedback only for confirmed native hits | HP delta, attack contact, timing and controller slot | Visual/audio/rumble feedback | Feedback suite; live confirmed versus blocked/missed hit comparison |
| `launcher.h`, `launcher_render.h` | Place guest-owned launcher and bridge native camera/motion | Config placement, consumed interaction, SDK launcher state/mesh | Scoped aiming/launch control and rendered world props | Core launcher and adapter tests; live pivot, multi-direction launch and cancel |
| `starting_class.h`, `class_preview.h` | Map native class record to persistent guest intent | Native class/stats/menu and preview targets | Class text, animated guest preview, saved selection | Class/presentation suites; guarded creation submenus and save reload |
| `render_device.h` | Implement neutral texture/camera/mesh interface in D3D9 | RGBA8, packed vertices, matrices, render phase and depth availability | Managed atlas, transforms, triangle submission | Real synthetic D3D9 upload/matrix/depth checks; production host compile |
| `render_surfaces.h`, `render_composite.h` | Capture scene depth and composite guest at correct size | Device surfaces, viewport, render target and depth | Depth-tested guest layer, restored device state | Synthetic multi-resolution occlusion/state/reset tests; live resolutions |
| `ui_order.h`, `ui_render.h` | Recognize native before-HUD boundary and render diagnostics | Native vertex declarations, blend/depth state | Guest below menus/text, optional debug HUD | Synthetic UI order/failure retry tests; live subtitles/menu priority |
| `diagnostics.h`, `report_files.h`, `debug_view.h` | Capture bounded bug context without exposing native pointers | Neutral observation, host ID, build identity | Local review, diagnostic bundle and prefilled issue URL | Diagnostics C++/Python suite; manual review/cancel/submit |
| `harness.h`, `harness_policy.h`, `qa_arsenal.h`, `qa/` | Guard repeatable native QA and scoped fixture equipment | Explicit test identity, heartbeat, current control handoff | Disposable test actions or rejection | QA CLI/import tests, fixture guards; never owner's level-65 character |
| `install.py`, `uninstall.py` | Edition-specific validation/configuration and transactional install | Exact build/test receipts, executable hash, private asset manifests | Backed-up install or rollback/restoration | Isolated installer/uninstaller/release tests; closed-game install only |

`startup_hash.h` is retained legacy hashing code and is not the current executable validation gate. Native calling conventions include x86 register-based/naked hooks; they must be
reimplemented rather than widened mechanically. Do not share pointer offsets,
packet layouts, hard-coded behavior/animation/equipment IDs or map archive names
with core or adapters. SDK coordinates use local centimetres; host observations
and motion mailbox/support policies use metres, with explicit host conversion.

The simulated-host suite exercises policies without these native integrations.
Synthetic D3D9 tests use only generated geometry, not a game process. Live verification
remains required after any new host implementation. See [host preparation](../../docs/HOST_PORTING.md).
