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
