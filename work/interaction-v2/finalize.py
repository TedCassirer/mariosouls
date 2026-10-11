from pathlib import Path
import json,hashlib,shutil,configparser,datetime
w=Path(__file__).resolve().parents[1];out=w.parent/'outputs'
g=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA');base=g/'MarioSouls'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(g/'dinput8.dll')=='881ed7fc251a79090f2c4a31901d2a2a44b2676a25518ba60c7d2de740a212bf'
assert sha(base/'sm64.dll')=='888e68f5878710ff39631df9d4331559894acc27550470292c70193cb5041d75'
for name in ['action_input_test.cpp','mario_action_input.h','mario_action_runtime.h','mario_action_effects.h','mario_flask.h','mariosouls.cpp']:
 shutil.copy2(w/name,base/'source'/name)
shutil.copy2(w/'libsm64/src/libsm64.h',base/'source/libsm64.h')
m=json.loads((base/'install.json').read_text());m.update(version='0.7-contextual-actions',controller='XInput: Y contextual interaction, X selected item; A jump/B attack/LT crouch in Mario; normal native menu controls',interactions='Same Mario instance throughout; bounded Y/Q interaction and X/E goods requests; confirmed native scripts own action displacement; phase-based Mario poses with blends; no human locomotion mode',interaction_validation={'date':datetime.datetime.now().isoformat(),'character':'test (level 4)','verified':['direct Estus use and native healing','bonfire sit/rest/stand/resume jump','ladder down/pause/up/dismount/resume jump','locked west-door response','system menu close/resume','rejected interaction retains movement','bloodstain recovery','ordinary native incoming damage and death/respawn','outgoing punch damage'],'limitations':['Fresh NPC conversation not replayed: Oscar already dead','Physical controller disconnected; controller input policy tested in executable','Other items and area-specific scripted actions not exhaustively tested','Native healing-body effect retained; flask and spill at human-hand dummy replaced']})
m['source_sha256']={str(p.relative_to(base/'source')).replace('\\','/'):sha(p) for p in sorted((base/'source').rglob('*')) if p.is_file()}
(base/'install.json').write_text(json.dumps(m,indent=2));(w/'prototype-install.json').write_text(json.dumps(m,indent=2))
cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(g/'mariosouls.ini');cfg['Test'].update(Mode='0',Frames='0',Buttons='0',X='0',Y='0');cfg['Harness'].update(Armed='0',Session='0');cfg['MarioSouls']['UITrace']='0';cfg['World'].update(Trace='0',Debug='0')
with (g/'mariosouls.ini').open('w') as f:cfg.write(f)
hc=configparser.ConfigParser();hc['Harness']={'Armed':'0','Session':'0'}
with (base/'harness.ini').open('w') as f:hc.write(f)
p=out/'MarioSouls controls and notes.txt';s=p.read_text(encoding='utf-8').replace('experimental local build 0.6','experimental local build 0.7')
s=s.replace('Y (top face button): interact with the nearby object. The normal game prompt still shows A.','Y (top face button): interact with the nearby object. The normal game prompt still shows A.\nX (left face button): use the selected item directly, including Estus. No Y press first.')
s=s.replace('Ctrl: crouch. Press Ctrl while airborne to ground pound.','Ctrl: crouch. Press Ctrl while airborne to ground pound.\nQ: interact. E: use the selected item directly, including Estus.')
start=s.index('INTERACTIONS WHILE STAYING MARIO');end=s.index("Mario uses the actual libsm64",start)
s=s[:start]+'''INTERACTIONS WHILE STAYING MARIO — 0.7
Y / Q requests the nearby action. X / E uses the selected item. These buttons no longer enter a human movement mode. If an action cannot start, you retain Mario movement.
Mario remains the same character through an accepted action, with blended drinking, sitting/standing, climbing or door poses. Movement resumes when the actual action finishes. Dark Souls controls item charges, healing, keys, interaction requirements and world changes.
On ladders: left stick up/down, or W/S. Climbing animation follows actual travel and pauses when you stop. Mario resumes after dismounting.
At bonfires and in menus: native menu controls, A to confirm and B to back out; Enter on keyboard. Leave the bonfire, let Mario stand, then jump normally.
Start / End opens the normal menu. Closing it returns to Mario. The interface continues to draw over Mario.
F6 or the stick-click shortcut remains the manual Mario on/off switch; Y is exclusively an interaction request. Avoid manually switching while attached to a ladder.

Verified on the test character: direct Estus, native healing and one charge, bonfire rest/leave/jump, ladder down/pause/up/dismount/jump, a locked-door message, menu close/resume, walking after a rejected request, and bloodstain recovery. Native incoming damage and death still work. See "MarioSouls interaction update.txt" and the accompanying demonstration.
Fresh NPC dialogue could not be replayed because Oscar is already dead on this save. NPC scripts use the same native action route, but that flow needs a live conversation test. Other items and special scripted interactions have not all been checked. Mario's adapted poses do not exactly match every native handle or ladder rung.

'''+s[end:]
p.write_text(s,encoding='utf-8')
(out/'MarioSouls interaction update.txt').write_text('''MarioSouls 0.7 — contextual interactions

Installed: Y/Q no longer switches you into a human locomotion mode.
Use Y/Q to interact, and X/E to drink Estus or use the selected item directly.
Mario keeps the same instance throughout, blends into action-specific poses, and resumes his own movement as soon as the accepted action ends. Requests that do not start an action leave movement available.

Estus has a right-hand drinking pose and an attached flask. The duplicate human flask/spill visuals are suppressed; the native item action still consumes the charge and supplies healing. Bonfire poses follow sit, rest and stand phases. Ladder animation follows vertical travel, including stopping and changing direction. Native doors and NPC scripts remain responsible for their own requirements and effects.

Verified in the actual game on the level-4 test character:
- Direct keyboard Estus: native 135 -> 435 HP in the first integration check; later 549 -> 594 capped at maximum. One use consumed one charge. A held E did not repeatedly consume flasks.
- Bonfire rest, Leave, standing and immediate Mario jump.
- Ladder mount, down, stationary pause, up, bottom dismount and jump.
- Native "It's locked" response at the west upstairs door, with Mario visible and text above him.
- Native System menu open/close, and Mario movement after a rejected interaction.
- Punch dealt 44 HP to a hollow. An ordinary enemy hit dealt 45 HP to Mario. A subsequent pursuing hollow killed the test character; native death and respawn worked, and all 184 dropped souls were recovered normally in Mario mode.
- Input policy executable checked controller routing, camera preservation, ladder/menu controls, request expiry, cancellation and separate keyboard/controller pulses. Runtime animation checks confirmed unchanged run/jump/long-jump behavior and same-instance resume.

Limits:
Fresh NPC dialogue remains unverified: Oscar was already dead on this save. No NPC, quest, inventory, level or save-state edits were used to recreate dialogue. The controller was disconnected during this test pass, so the new X/Y routing has code-level checks but no new physical-controller confirmation. Other items, every scripted door and a complete playthrough remain untested. Adapted Mario poses do not exactly fit every native rung or handle.

The level-65 character was not loaded. Runtime and save backups are retained. Restore original game.cmd remains available and hash checked. Current controls are in MarioSouls controls and notes.txt.
''',encoding='utf-8')
print('Final version and source hashes written; controls and verification notes updated.')
