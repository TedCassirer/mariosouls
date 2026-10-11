# Native equipment parry and critical ownership, 2026-10-11

Framework 0.7.0: optional SDK native_combat capability, native combat pose and
equipment action kinds. Shared core policies handle fresh input and observed
native completion. PTDE owns routing, native animation tracking, root following,
eligibility hints and lifecycle cleanup. Mario's adapter owns all authored
shield/critical geometry; no character-specific branch was added to the host.

RT/Tab requests shield parry. Normal attack behind an enemy or facing an actually
parried target requests native critical eligibility; guard+attack is an explicit
attempt. Native DS resolves the result. No guest damage or stamina writes run
during this ownership. Focus loss cancels queued input; native busy actions are
not truncated by a timeout. Menus, death, actor/map loss and held buttons retain
their lifecycle guards. Unsupported gear fails closed.

Replaced binary search of native parameter row directories with validated index
caching because owned live data is not globally sorted. Expanded offline tests,
SDK/adapter docs, and guarded QA capture helpers. See docs/VALIDATION.md for actual
native HP, stamina, soul and animation measurements and remaining limitations.

# Death presentation, 2026-10-11

Baseline c64a462 / immutable contextual-A checkpoint. The immediate HP<=0
release destroyed Mario before any death animation could render. Added a shared
NativeDeath lifecycle state and optional ABI4 death_pose capability. The PTDE
host releases motion/capsules and transient combat/input, retains only the guest
presentation, and lets native HP/death animations 6000/6100, bloodstain creation,
loading and respawn proceed. Identity loss or native revival destroys the old
guest; the existing living/idle stability gate can recreate it.

Mario uses owned-ROM animation 0x32, frames 0..95, with the original dead eyes
and one dying voice. A narrow staged-runtime patch gives host pose style 8 a
nonlooping terminal frame. Guest death actions/warps are never executed. Existing
runtime source and prior checkpoint archives are unchanged.

Offline: full supported build/test/doctor, two ROM-backed death lifecycles,
monotonic frame progression, stable final mesh, one-shot voice callback, cleared
attack/interaction/socket flags, recreate/resume jump and session ownership tests.
Live: Mario QA only; native melee HP 594->0, visible collapse/final pose under
YOU DIED, native respawn HP594/souls0, automatic Mario reactivation and 2.10 m
observed jump rise. Recovered all212 souls through the actual bloodstain prompt;
bonfire rest/Leave passed and the character was saved normally with HP594/souls212.
Early bonfire requests were outside its active prompt after respawn; repositioning
to the existing fixture restored the prompt. They were rejected without handoff.
Evidence work/death-qa, preview outputs/Mario death animation.mp4.

# Framework equipment work, 2026-10-11

Baseline: 0.4.2, d3f03f6, immutable checkpoint runtime archive and tag retained.
User approved a first one-handed sword and shield implementation, preserving
Mario's existing moves when unarmed. All shared integration remains in framework.

Route: keep the embedded guest as locomotion owner. Extend the SDK with render-only
equipment poses and rigid hand sockets from the same frame as the mesh. Host owns
equipped item selection, native weapon damage, stamina and shield guard. The Mario
adapter owns arm fitting; no host branch on character ID. Existing item-use/full
body pose handoff is unsuitable for weapons because it resets movement/action.

Implemented in 0.5.0 / ABI4: optional equipment presentation and same-frame hand
sockets, shared swing windows and swept blade contact, and the PTDE native item,
reinforcement, behavior, stamina and damage route. The host validates the packet's
weapon/hand so prior shield state cannot silently substitute the off-hand item.
The initial scope is one-handed straight swords, small/medium shields, two
alternating ground light swings and blocking. Empty right hand retains Mario's
independent attacks. Parry, two-handed moves, other weapon families and temporary
buff parity are not verified.

Mario layers fitted arms onto the normal guest animation without an extra physics
tick. The host commits movement during a sword swing and slows movement while
guarding. Native block rules remain native: no post-hit healing or invented
protection flags. Hurt and native interactions invalidate equipment sockets, so
props may temporarily disappear in those poses.

The private asset pipeline applies bone rest transforms and converts installed
PTDE parts/textures with Soulstruct/Pillow. It prepared 47 models (13 straight
swords and 34 small/medium shields), mapped from 304 native equipment rows.
Generated geometry, textures and captures remain outside Git. The supported
prepare-equipment/install workflow validates manifest hashes, executable identity
and bounded binary geometry, copies transactionally, and retains the configured
equipment directory on later installs.

Offline checks cover shared swing/contact policy, asset layout and rejection of
malformed inputs, transactional installation, and Mario's paired movement/action
timing with and without the render overlay. Mario-specific checks include visible
arm-pose changes, finite rigid sockets, correction/rebasing, interaction/hurt
invalidation and armed idle/wake behavior. These checks do not prove native
blocking or damage by themselves.

Initial Mario QA observations: Straight Sword Hilt reduced a 44-HP target to 31
(13 actual HP damage), with 20 stamina charged and native packet physical power
20.26. An empty-hand Mario attack killed a 44-HP target with 9 stamina charged.
Shield checks measured 33 HP chip and 32 stamina drain from the front, versus 65 HP
damage per rear hit without guard stamina drain. These are individual observed
cases, not general balance values. The blocked-chip hurt/pause presentation still
needed correction at this checkpoint; final regressions and their evidence belong
in docs/VALIDATION.md.

Final correction: read current saved-options keyboard bindings, observe exact
resolved native block/guard-break packets, and require incoming evidence before
native passive ownership. Corrected rendered handedness in the Mario adapter.
Final live guard, chip, rear hit, sword, exhaustion and unarmed checks passed;
see docs/VALIDATION.md for the measurements and explicitly unverified cases.

Final live regression scope: guest movement with/without equipment overlay;
actual right-hand weapon visible and swung; native target HP/stamina traces;
front shield block and rear/unblocked damage; exhaustion/guard break; unarmed
attack restore; equipment change and native interaction lifecycle; debug view;
Mario QA only, backed-up saves and current user control handoff. Failure to prove
native guard or weapon ownership is not a functioning feature.
# Fire Flower / native pyromancy, 2026-10-11

Framework 0.8 adds an optional ABI4 pyromancy capability and native left-light
input pulse. Either hand can request a native cast, with guest contacts suspended
until native recovery. The Mario adapter owns the procedural flower, material
color changes and original fitted throw poses. No character switch was added to
the DS host. Existing native guards and source-specific input release remain tested.

Mario QA received a base Pyromancy Flame and Fireball through the guarded native
acquisition helper, then attuned/equipped them through actual game menus. Saves
were backed up first. Native HP loss and death occurred during an early hostile
test setup; that interrupted cast is not counted as a pass. Three-metre rear
placement produced a native baseline kill and matching Mario kills from each hand.
See docs/VALIDATION.md for evidence, final-build checks and limits.

## Wider camera, framework0.8.1

Added optional SDK camera-profile export; Mario's manifest requests1.5 distance.
The host scales the38 live LockCamParam distance fields only while a character
owns the session, with exact compare-and-restore and stale-directory checks.
Native camera interpolation, wall collision and lock-on remain in charge.
Observed courtyard framing moved4m to approximately6m; confined lock-on retained
native proximity constraints. All38 fields restored on toggle-off. Configuration
is `[Camera.mario64] DistanceScale`, range1..2. See validation for measured scope.

## Mario starting class, 2026-10-11

Added optional SDK starting-class profile, shared bounded validation and one
activation decision per native actor. PTDE owns the class-7 mapping, FMG text
hooks and private copies of CharaInitParam rows 2007/3007. Mario owns the class
description and preview framing. Native save/inventory initialization remains
authoritative; existing Pyromancer saves receive no retroactive equipment.

Live creation checks: class list and selected-class field display Mario;
description and Flame/shield preview equipment agree. A real animated guest mesh
replaces only the full-body native preview texture. Switching to Sorcerer restores
its human preview, and returning to Mario recreates the guest. Full capture is
1920x1080 but client coordinates are 1280x720: clicks from a 960x540 capture must
scale by 4/3. Incorrect scaling previously clicked outside the window.

The human created Mario ClassQA after both Unicode input and main-window WM_CHAR
failed in native name entry. Read-only observation verifies native class 7,
level 1, attributes 10/12/11/12/9/12/10/8, HP 573, stamina 93, right Pyromancy Flame
1330000 and left Plank Shield 1406000. Native empty hand slots resolve initializer
-1 to unarmed item 900000. Mario automatically owns the world session. Screenshot
shows Fire Mario and attuned Fireball. No acquisition helper or inventory writes
were used. Existing character guards remain unchanged; qa/starting_class.py
observes only the exact new level-1 fixture in the Asylum.

Offline checks pass: class profile rejection, activation/manual override/reload,
real preview mesh animation/rotation/repeated teardown, subsequent gameplay
instance and the existing framework/adapter regressions. Final preview-facing,
name-modal and restart checks are recorded below when completed.

User correction after the initial pass: retain the native starting inventory and
obtain the Flame through the original tutorial pickup. Removed the entire native
initializer hook instead of compensating with later inventory edits. Earlier QA
save inventory evidence above describes the superseded build; existing saves
are not stripped of previously acquired items. Original row 2007 remains intact.
The creation appearance menus also select different native preview textures;
expanded preview coverage is undergoing a separate live pass.

Final portrait diagnosis: both panes use the original UP sprite path; the actual
difference is the quad inset. Clothed panel bounds are 6600,1216,9568,4984;
appearance panel bounds are 6720,1720,9576,5056. The earlier exact-rectangle
filter excluded the latter. Removed unsuccessful buffered-draw hooks. Final
matching requires a large right-pane quad and native 512-square ARGB render
target, with aspect correction per quad. Native face-editor +B0 state keeps
the guest alive in advanced customization.

Live verification: face, hair and color dropdowns and advanced customization
all retain animated Mario. Physique list/selection also retains Mario. His
standard outfit is unchanged by human appearance sliders. Native initializer
D1FA40 retains its original bytes in the running build after removal of the
equipment override. Class-7 tutorial routing remains native.

Final installed-build pass: corrected per-pane aspect and native viewer rotation;
Mario remains visible in name entry/dismissal and face/hair panes. Sorcerer
selection restores the native human, switching back restores Mario. Full build,
offline regression suite and doctor passed. No new combat claims are made by
this menu test. Final captures are outputs/Mario class.png and
outputs/Mario appearance preview.png. The earlier Mario ClassQA save predates
the requested inventory correction and intentionally keeps its acquired gear.

## 2026-10-11 - Class activation loading gate

User reported loading a Mario-class save as human. Read-only live observation
confirmed saved class 7. The installed log contains the automatic class request,
then a controller toggle cancelling that pending request while still human; a
second toggle activates successfully. No load/floor failure was logged there.

Fixed framework session stability: the 500 ms timer follows native actor,
physics and map identity instead of requiring uninterrupted idle animation.
Unsafe frames still prohibit ownership, but no longer reset identity stability.
Class selection keeps observing until matching saved data arrives and preserves
explicit manual decisions. A toggle while human enables the guest even with
an automatic request pending. Existing manual-off behavior after activation
is retained. No save, inventory, health or class-byte writes were added.

Build, complete offline framework/real-adapter suite and doctor passed. Tests
cover delayed class data, manual suppression, identity changes during loading,
unsafe frames and first-safe-frame activation after settling. Installation and
live reload validation await the requested control handoff; the running game
is still the previous checkpoint. This entry is not a live verification claim.

## 2026-10-11 - Cannon aiming correction (prepared, not installed)

The user video shows a follow-camera orbit and unchanged shot direction. The
right stick previously bypassed cannon input and controlled the native camera.
Added either-stick aiming with right-stick priority, a 150 ms native camera-input
lease, and corrected the Mario adapter horizontal raw-stick sign. The original
cannon actions and flight remain unchanged. Real-adapter regression tests now
compare opposite aimed headings with muzzle displacement and launch velocity,
including release and a host motion acknowledgement.

Read-only examination of the running game found the gameplay camera vtable
1174114 but render handle +4 was zero. Existing FF7460 submission interception
therefore did not override that gameplay camera. Native 404EA0 exports its
camera-to-world matrix using ECX source and EAX destination, returning ECX+40.
Replaced the submission hook with a signature-checked export wrapper. It changes
only copies from the current gameplay camera, under the actor-bound expiring
request; the source matrix is never written. Guest drawing uses that same
exported eye/forward. A dedicated executable test invokes the actual x86 wrapper
and tests register results, original preservation, unrelated cameras, stable
pivot across yaw, expiry, death, actor change and cancellation.

Full build, full framework tests and doctor pass. The hash-pinned local installer
passes read-only check-only verification and refuses an open game. Includes
1445508 Mario load gate fixes. No install or game controls performed: user chose
prepare-only and will test personally. Live scene composition/pivot, controller
feel and terrain trajectories remain unverified for this revision. No new
checkpoint tag replaces the earlier verified Mario-class checkpoint.
