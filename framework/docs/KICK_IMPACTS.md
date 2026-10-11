# Poise-aware kick impacts

Status: framework 0.8.3 installed and verified on Mario QA. The previous 0.8.2
checkpoint is preserved. Native enemy reactions determine actual travel, so
requested metres are tuning values rather than a guaranteed displacement.

The host already had private per-attack native rows and a verified pre-defense
packet hook. Extend those paths rather than applying enemy position or velocity
writes. The optional impact SDK export lets each adapter select its own attacks.

Initial Mario tuning:

| Move | Poise damage | Maximum requested push |
| --- | ---: | ---: |
| Combo-finishing kick | 30 | 1.40 m |
| Jump kick | 25 | 1.15 m |
| Slide kick | 35 | 1.60 m |

Requested distance decreases linearly with maximum poise, reaching zero at 80.
Current poise must be below the attack's poise damage before a push is requested.
Native defenses and reaction rules may further reduce or reject the request.
A capsule radius above 0.80 m or total height above 2.80 m blocks added push even
when the creature's numeric poise is zero. This matters because native boss
resistance is also expressed by effects rather than solely by the poise number.

Each enemy can receive an enhanced push request at most once per 1.2 seconds.
Punches do not spend this cooldown. Existing stamina costs, HP attack power,
one-hit-per-attack windows, weapons, stomps and projectiles are unchanged.

Evidence used for implementation:

- Local Soulstruct ATK_PARAM_ST defines knockbackDist at row +0x10 and dmgLevel
  at +0x72. NPC_PARAM_ST documents passive reaction-remapping effects.
- Owned PTDE executable D20122..D20145 copies row +0x10 into damage packet +0x54.
  Existing packet hook D1FE20 already supplies independent character attack power
  and poise damage at packet +0x28. Reaction level remains native light impact.
- The repository's DS-Scripting Chr definition identifies current/max poise at
  actor +0x1c0/+0x1c4. Live traces must confirm plausible values and responses.
- Existing native capsule reader is used read-only for body bounds.

Build, full framework tests and doctor passed. Added tests cover invalid profile
data, poise thresholds, resistance scaling, large bodies, cooldown reuse and clock
wrap. Real adapter export rejects incompatible version/size requests.

## Live evidence, 2026-10-11

Installed build backup: before-framework-20261011-155552-689317. Save snapshot:
work/kick-qa/saves-before. All combat checks used Mario QA, level 13, Asylum.
Initial title-menu navigation inadvertently loaded the level-65 main character;
identity inspection caught this before combat tests or test writes. Returned via
native Quit Game and visually selected Mario QA. Test identity guards remained
enabled. Future launch navigation must inspect each screen instead of batching
confirmations across loading transitions.

Used temporary FixedPower=10 to keep the hollow alive during reaction checks.
Recorded native positions, poise and animations; no enemy position, HP, AI or
poise writes, no forced reaction animations. Hollow 1810103 reports poise 0/0.

- combo1: opening punches did not suppress the finishing kick. Native reaction
  2001 moved the hollow from Z=-4.352 to -2.680, about 1.67 m during the kick
  reaction. HP 69 to 68. It recovered and resumed AI, including an attack.
- slide1: native reaction 2002 moved the hollow from (-7.760,-5.000) to
  (-7.450,-3.202), about 1.82 m horizontally. HP 69 to 68, then AI resumed.
- jump3: isolated jump kick requested 1.15 m; native reaction moved the hollow
  about 0.55 m. At this reduced attack power, damage rounded to zero.
- jump-normal: restored FixedPower=0. Jump-kick attack power 116.375, stamina
  cost 19 including the existing equipment penalty; hollow HP 69 to 3. Native
  reaction 2002 again moved it about 0.55 m, then normal AI resumed.
- boss1: Asylum Demon reports poise 0/0 but has a 2.50 m radius, 7.50 m height
  capsule. Combo kick requested 0 m and dealt 1 HP at reduced test power. The
  boss continued attack 3008 instead of entering a knockback animation. Its
  ordinary attack movement is not counted as knockback.

Earlier jump1 overlapped an automatic stomp and is not isolated jump-kick proof.
jump2 was invalid because the hollow had chased away from its fixture; the HP
guard stopped the run and the helper retreated. Added a fixture-location check
and adjusted input spacing for the 200 ms command poll; rested before rerunning.
Recorded contact sheets were inspected. Close camera framing is a fixture limit.

Only Oscar, a friendly NPC with 1 HP, had nonzero poise among loaded actors; he
was not attacked. Intermediate/high-poise scaling and rapid-repeat suppression
have offline policy coverage, not live armored-enemy coverage. Other bosses,
all-map wall/ledge cases and every defensive special effect remain unverified.
Native hit and collision rules remain responsible for those interactions.

Final state: normal attack power restored, right-hand Pyromancy Flame and left
Plank Shield selected, Mario QA rested and saved through native Quit Game. No
new runtime build was made after the live checks; archive hashes verify the
installed host and adapter against the tested build.

Verified checkpoint: checkpoints/framework-kick-knockback.json (92 runtime files).
Restore helper: outputs/Restore kick knockback checkpoint.cmd. Short diagnostic
video with game audio: outputs/Mario kick knockback check.mp4; final contact sheet
inspected. The prior bouncing Fire Flower checkpoint remains immutable.
