---
kind: game
title: "Playable libsm64 Mario in Dark Souls PTDE"
game: "Dark Souls: Prepare to Die Edition"
games_also: ["Super Mario 64"]
game_version: "Steam build 1480646, executable 1.0.2.0, x86"
platform: windows
engine: unknown
route: native-hook
tools: ["libsm64 fd11813208272b4271d92bd92feb8f3fdbe61be5", "Zig 0.13.0", "MinHook", "Soulstruct", "Universal Modder 0.2.0"]
anti_cheat: "Offline single-player testing; no multiplayer bypass"
status: in-progress
agents: ["Codex (GPT-6)"]
humans: []
date: 2026-10-10
links: ["https://github.com/libsm64/libsm64", "https://github.com/JKAnderson/DS-Gadget", "https://github.com/Wulf2k/DarkSoulsScripting", "https://github.com/PeterTh/dsfix", "https://github.com/Grimrukh/soulstruct", "https://blog.gib.me/2014/12/21/dark-souls-no-logo-hack-steam-build-2-0/"]
tags: [mashup, libsm64, d3d9, collision, startup]
---

# Playable libsm64 Mario in Dark Souls PTDE

Private experimental integration of actual SM64 movement, animated geometry and audio into the owner's PTDE installation. A new Asylum test character was used. Jumping, movement, punches, ground-pound animation, native enemy damage and a kill awarding souls were verified in the actual game. Broader progression and dynamic geometry remain unfinished.

## Setup

Windows, Steam app 211420/build 1480646; executable SHA256 67bcab513c8f0ed6164279d85f302e06b1d8a53abff5df7f3d10e1d4dfd81459. Native x86 dinput8 proxy, MinHook revision 8af6b4acae5a9388fd742b56fa79ece89d96f823. Owned US big-endian 8 MiB SM64 ROM, SHA1 9bef1128717f958171a4afac3ed78ee2bb4e86ce. Save/config snapshots made before testing. Windowed 1280x720, keyboard/mouse.

## Route and why

Embed libsm64 as a library and synchronize its player with the native character. Native engine owns camera, enemy health, souls, saving and progression. D3D9 draws the ROM-derived animated mesh. No replacement game executable or extracted game archives are installed.

## How the game works (what we had to learn)

DS-Gadget/Scripting pointer chains resolve player, camera, map and stats. Pointer traversal means dereferencing at each offset. Native player movement controller has a warp request; updating only its position block is insufficient for the camera and physics. Dark Souls metres are converted to 100 SM64 units, with a local origin and reversed triangle winding.

BHD5 archives contain MSB map instances and split HKX collision binders. Soulstruct exports private static triangles. The Asylum is m18_01, not m18_00 (Kiln). A cached world depth surface permits occlusion when rendering Mario into a transparent 1024x720 layer, then compositing at EndScene.

The native hit wrapper uses behavior IDs 530/531, which map to attacks 40/41. The script attacker ID 10000 is a special player resolution; the player entity's own ID is FFFFFFFF. Actual enemy damage and souls must be observed to validate this path.

For this executable, startup phase function 0xEE5900 requests logo playback through MenuMan +10C. Calling the original phase and clearing only that request skips logos without modifying clocks. The full function prefix is verified before hooking. This adapts behavior identified by gibbed's older No-Logo tool to the current build; that old DLL is not installed.

## Build steps

Compile libsm64's C sources and the custom adapter for x86 with Zig. Link D3D9, WinMM, user32 and MinHook. Generate collision caches from the owner's local game files. Install the custom proxy beside DARKSOULS.exe and the runtime under DATA/MarioSouls. Set owned ROM and runtime paths in mariosouls.ini. F6 activates/deactivates; F7 resynchronizes.

Keep the private sources, dependency revisions and install hashes with the runtime. The user-facing launcher matches screenshots of the existing test character before confirming that slot. A hash-checked restore script removes only the matching custom proxy and preserves saves/backups.

## Verification

Observed Asylum gameplay and logs: 752-triangle textured Mario, camera following, world depth occlusion, jump >2 metres, punch and ground-pound animation. Hollow 1810101 took 44 -> 30 HP from a jump/stomp, then 30 -> 16 -> 2 -> dead from moving punches; native game awarded 20 souls. A 13.47-second clip includes game audio. Standalone libsm64 long-jump and 5000-frame sleep/wake tests pass. Extended in-game idle/wake stayed stable.

Direct startup-hook clean test reached the test character in 32.8 seconds before adding automatic acknowledgement of the autosave notice. The final installed PowerShell launcher then completed a clean launch in 23.1 seconds, confirming the level-4 test character. Mario activation, a short jump/landing and rendering were rechecked after that launch. Earlier timing methods were abandoned. Restore verification checks pass without changing files.

Not verified: full playthrough, all map caches, long-distance rebasing, death/warp recovery, incoming-damage animation, wall kicks/triple jumps and all attack variants in the host game.

## Gotchas

1. **Camera remains behind.** Position block writes are overwritten. **Fix:** use the native movement controller warp request as well.
2. **Mario loses torso or has invisible regions.** SM64 texture alpha mixes texture colour with vertex colour. **Fix:** blend these channels instead of discarding transparent texels.
3. **Sleep/wake hangs.** libsm64 sound lists were never initialized. **Fix:** initialize audio and pump audio ticks even during idle.
4. **Hit calls do nothing.** Attack IDs were passed where behavior IDs are required. **Fix:** verify the mapping and enemy HP/kill outcome.
5. **Native scene pass is invisible or wrong.** A guessed render-pass ordering did not match this setup. **Fix:** inspect the frame's actual surfaces and composite with cached world depth.
6. **Logo skipping still takes time.** Clock acceleration and native time-step experiments were unreliable. **Fix:** version-checked startup request hook. Shortening/removing the WMV was not retained; original movie restored.
7. **Automated exit fails.** A long End hold can toggle the menu twice; changing input from another source can also disrupt navigation. **Fix:** short taps, inspect Quit confirmation, verify title/null player before closing.
8. **Loader matches a black frame.** Whole-rectangle error tolerates missing sparse white text. **Fix:** also compare the reference's bright text pixels. Capture reference images as immutable files, not a live screenshot that another capture can overwrite.

## Assets

Mario geometry, texture, animation and audio are extracted at runtime from the owner's ROM. Collision is converted privately from the installed game. No AI-generated art, redistributed ROM, or publicly shared extracted assets. Notes and code are private; no public PR was opened.

## Open questions

Dynamic doors, breakables, ladders and elevators require native F6 mode. Fifteen static map caches exist, but Darkroot has an unresolved collision reference and Great Hollow/Ash Lake has an unsupported collision shape. These two maps refuse Mario activation. Wider gameplay QA remains necessary.

## Controller follow-up

The first adapter only read keyboard/mouse, so an XInput pad moved the native camera but could not control Mario. Build 0.2 dynamically reads XInput slot 0 (Xbox 360-compatible device on this machine), normalizes the left stick with a radial deadzone and maps A/B/LT to libsm64 jump/attack/crouch. L3+R3 held for 600 ms toggles Mario, with release gating after focus loss or disconnection; Start returns to native controls for menus.

Native XInputGetState and optional ordinal 100 are filtered while Mario owns the controller: left stick, A/B and LT do not also trigger native gameplay. Right stick remains available to the native camera. The mod polls through the original trampoline with a thread-local bypass so chained XInput exports cannot filter its own input.

Actual in-game physical controller inputs produced movement, jumps/double jumps, moving punches, crouching and a backflip. The user confirmed movement and buttons work. Both hooks installed successfully and actual native polling through the filter was logged. Pure input tests also covered deadzones, partial speed, negative-axis extrema, button thresholds, diagonal normalization, camera preservation and toggle/focus/reconnect state. Physical unplug/replug and the complete controller-only interaction sequence remain untested.


## Visible Mario interactions — build 0.3
- User rejected switching visually to the human. Added a separate interaction mode: release native movement/gravity restrictions, retain Mario instance/rendering, keep native player opacity zero, and follow native position/facing with animation-only libsm64 output.
- Local libsm64 export sm64_mario_pose_tick sets pose and generates geometry without bhv_mario_update or Mario physics. Avoids running SM64 door/pole actions that expect SM64 world objects. Both sm64.dll and dinput8.dll installed together; prior 0.2 binaries and two save files backed up in work/interaction-backup. No save restoration or progression memory edits.
- Read-only native recon: movement+18 has 29 animation records at stride 9C; +0 ID, +4 token; FFFFFFFF token is inactive. Track14 idle IDs can be 0 or 100000 (stance), normalized modulo100000. Track23 ladder IDs7010–7040; track24 forced events. MenuMan13786D0 fields mirror native IsShowMenu C2ABE0; omit +58 action prompt and +F0 banner. Bonfire menu observed +40=2; +210 alone was not reliable. Grounded return checks local static floor, native idle, no menu/event/ladder, 350ms stable. No timeout overrides a ladder pause or open menu.
- Mario pose mappings: 7000–7099 pole climbing05 (rest7040 uses0D), bonfire7700–7749 seated85 frame0, lighting7698 warming19, fog7410 walking48, other event7000–8999 door push60. Pose mapping is approximate, not retargeted native animation.
- Y/Q starts interaction; Y is withheld from native weapon stance changes. Native controller A pulse starts700ms after handoff and lasts250ms. Earlier100/250ms pulses were too early after Mario's floor sat20cm above native ladder floor. Update native XInput packet number when the filtered/synthetic gamepad changes, including a stationary physical controller. Start/End menu transition keeps Mario visible. Return recreates Mario on a checked floor and preserves original opacity for later toggling.
- Independent tests: ReturnGate missed request, paused ladder600sec, bonfire/standup, unsafe floor, focus loss, DWORD wrap; controller mapping/filter tests; owned-ROM pose smoke: five pose IDs752 triangles, finite vertices, fixed5000 height for45frames, animation progress; movement/jump/longjump/audio tests also passed. These do not replace live QA.
- Live QA final PID40632: Q140ms mounted first ladder after delayed pulse; tracks7010,7011/12,7040; paused at(-.681,192.457,22.362), remained attached, then7014 dismount and automatic Mario return; jump afterward verified. Descending from(0,196.09,21.7), heading2.3562, produced7021/22,7024 then Mario return at189.49. Earlier descent setup at(.2,195.99,21.1) was outside native prompt range; native mode also did not activate there.
- Bonfire actual game: native7720/21/22 (also stance7710/11/12), seated Mario/menu visible, Enter Leave, automatic return, Space jump passed. Regular End menu kept Mario visible and returned automatically when closed. Courtyard gate1811105 native7112 opened, Mario stayed rendered and resumed; moved fromx-10.67 through door planex-9.34 tox-4.88 with Mario physics.
- Door recon correction:1811200 atz-21 is destructible boss floor, NOT main door. Test positioning there triggered Asylum Demon and caused damage to the test character; unsafe static floor correctly refused Mario activation. Returned test to courtyard. No boss victory, HP edit, or original level65 character access. Correct main door1811111 is at(3.186,198.078,-4.033); untested. Successful door QA used shortcut gate1811105 instead.
- Only Asylum objects above verified. No universal interaction, full playthrough, elevator, moving-platform, NPC dialogue, or new-map guarantee. Native controller/keyboard menu controls temporarily apply while interacting. Keyboard-only Q can need a second tap after settling because delayed synthetic action currently uses an attached XInput controller.
- Screenshot proof in outputs/Mario at the bonfire.png, Mario on the ladder.png, Mario opens the gate.png. Final sources/manifest refreshed; Restore -CheckOnly passed. Physical controller Y confirmation pending at documentation time. Automated game input handed back to user beside the bonfire.

- After automated input ended, the live log recorded additional user-initiated bonfire entry/exit sequences and a return to active Mario, followed by physical controller jump/long-jump inputs. Explicit user confirmation of the Y-button experience has not yet arrived. Installed final hashes: proxy0cc580f98e4625da47398dde05b9b3d0a6e1e55af098c70b5e749ae36099fc24; sm64e44db4993d42c1bddcf68b57371c18c1767d7d855b5eb523393d004e2ba3325c.

