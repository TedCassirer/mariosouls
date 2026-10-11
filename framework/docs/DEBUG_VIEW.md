# Framework diagnostic overlay

The current framework also provides **F11** for a reviewed bug-report draft,
independently of this overlay. See [bug reporting](../../docs/BUG_REPORTING.md).

F8 toggles the overlay. F9 cycles Combat, Terrain, Objects, All. F10 toggles
through-walls drawing. F6 still toggles the character and F7 still rebuilds the
guest session. Debug keys are focus-gated and edge-triggered; a held key when
returning to the game is ignored until released. The overlay also works with
native controls, allowing before/after capsule comparisons. It starts off by
default and hotkeys do not change the saved configuration.

| Color | What is actually being displayed |
| --- | --- |
| Cyan | Native player movement capsule, read from the live Havok shape |
| Pink | Native player's secondary contact capsule, with owner/type validation |
| Red / purple | Living nearby hostile / other actor movement capsules |
| Amber | The active character attack's host range and height limits |
| Green dots | A bounded grid accepted by the adapter's read-only contact predicate inside those limits |
| White cross | Contact followed by an observed native HP decrease (existing feedback receipts) |
| Small amber cross | Contact submitted to native damage, not proof of damage |
| Yellow | Live native movement contact point and normal, while the guest owns locomotion |
| Teal | Nearby triangles from the collision cache used by the character |
| Orange | Loaded native object origins; these markers are not object bounds |

Attack geometry remains for 650 ms after its window closes, labeled RECENT.
HP-confirmed/contact crosses remain for 1.5 seconds. Actor changes and map changes
clear them. An exhausted attack has no active sample volume. The host range is
not a guaranteed hit: facing, occlusion, per-target limits, stamina, native parts,
defenses and immunity can reject damage. The grid approximates a predicate;
it is not a continuous mesh. Debug sampling never calls commitContact or step.

**Limits:** these capsules are not native per-bone hurtboxes. Native enemy weapon
hitboxes, arbitrary object bounds and full dynamic object meshes are not mapped.
The terrain view is the guest collision cache, not all live Havok geometry.
Event-controlled barriers deliberately omitted from that cache remain native;
their local contact normals are visible when the player touches them. Missing
geometry must not be interpreted as proof that an object has no native collision.
Confirmed-hit crosses use the existing feedback system and require Feedback.Enabled.

The existing pre-HUD render boundary keeps game menus/text above all diagnostic
geometry. Debug drawing does not write depth. D3D state is restored by the shared
renderer. The text atlas uses a managed D3D9 texture; there are no new default-pool
resources requiring a Reset hook. Diagnostics add no game-memory writes and do
not enable protection flags, change stamina/HP, or alter attack decisions.

Optional `mariosouls.ini` settings, read at startup:

```ini
[Debug]
Enabled = 0
Layer = 0
ThroughWalls = 1
RadiusMeters = 12
MaxTerrainTriangles = 3000
```

Layer 0/1/2/3 = Combat/Terrain/Objects/All. Radius is clamped to 4-40 m and terrain
to 100-5000 triangles. Terrain refreshes at most four times per second and shows
CAPPED when truncated. Actor/object enumeration and all line buffers are bounded.
The log records view changes and counts, but nothing while the overlay is off.

Related tools researched: [DS Gadget](https://github.com/JKAnderson/DS-Gadget) is
a general PTDE testing utility; [DS Anim Studio](https://github.com/Meowmaritus/DSAnimStudio)
supports PTDE animation/event editing and viewing. Neither knows this framework's
custom guest attack predicate. No external tool, debug executable or game asset
was bundled to implement this overlay.
