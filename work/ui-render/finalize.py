from pathlib import Path
import configparser, hashlib, json, shutil, os

w = Path(__file__).resolve().parents[1]
g = Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
out = w.parent / 'outputs'
proof = w / 'ui-render/check-044538'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(g/'dinput8.dll') == sha(w/'build/dinput8.dll')
for name in ['mariosouls.cpp', 'mario_ui_order.h']:
    assert sha(g/'MarioSouls/source'/name) == sha(w/name)
s = json.loads((proof/'state.json').read_text())
assert s['player']['name'] == 'test' and s['stats']['level'] == 4
assert s['player']['hp'] == 594 and not s['menu'] and not s['interaction']
assert not any(s['protections'].values())

cfg = configparser.ConfigParser(); cfg.optionxform = str
cfg.read(g/'mariosouls.ini')
cfg['MarioSouls']['UITrace'] = '0'
cfg['Test'].update(Mode='0', Frames='0', Buttons='0', X='0', Y='0')
cfg['Harness'].update(Armed='0', Session='0')
tmp = g/'mariosouls-ui-final.tmp'
with tmp.open('w') as f: cfg.write(f)
os.replace(tmp, g/'mariosouls.ini')

p = g/'MarioSouls/install.json'
m = json.loads(p.read_text())
m['version'] = '0.6.1-ui-order'
m['proxy_sha256'] = sha(g/'dinput8.dll')
m['source_sha256'] = {p.name: sha(p) for p in sorted((g/'MarioSouls/source').iterdir()) if p.is_file()}
m['render'] = 'Mario composited once per presented frame before native interface sprites/text, with native vertex declaration and transforms restored'
m['render_validation'] = {
    'date': '2026-10-11', 'character': 'test', 'level': 4,
    'passed': ['bonfire prompt overlaps Mario correctly with readable text',
               'bonfire Leave menu readable', 'native system menu covers Mario correctly',
               'bonfire handoff returns to Mario', 'jump after leaving bonfire',
               'menus close and Mario control resumes'],
    'limits': ['Oscar dialogue was already exhausted on this save; NPC subtitle replay was not performed',
               'Other resolutions and areas were not newly tested'],
    'evidence': str(proof)
}
p.write_text(json.dumps(m, indent=2))
(w/'prototype-install.json').write_text(json.dumps(m, indent=2))
shutil.copy2(proof/'prompt.png', out/'Mario UI layering fixed.png')
shutil.copy2(g/'mariosouls.log', w/'ui-render/final-runtime.log')

notes = out/'MarioSouls controls and notes.txt'
text = notes.read_text(encoding='utf-8')
addition = ('UI DRAWING FIX (0.6.1)\n'
    'Mario now draws before native interface sprites and text. The bonfire prompt and system menu correctly cover Mario, and menu text stays readable. Verified on the level-4 test character, including resting, leaving and jumping afterward.\n'
    'The same draw boundary covers native text, but Oscar dialogue was already exhausted on this save, so a fresh NPC subtitle replay was not verified. The existing world depth test is retained.\n\n')
text = text.replace('CURRENT LIMITS\n', addition+'CURRENT LIMITS\n')
notes.write_text(text, encoding='utf-8')
with (w/'MODLOG.md').open('a', encoding='utf-8') as f:
    f.write('\n### UI rendering order repair v0.6.1 — 2026-10-11\n'
        '- Moved RenderMode2 compositing out of EndScene and onto the real device UP/IUP native UI boundary. Exact declarations, alpha/depth state and full-size A8R8G8B8 target identify scene-copy/sprite/text draws. Present resets the per-frame guard after the native/Steam overlay chain returns, preventing a second late composite.\n'
        '- The probe-device draw entry points missed native UI on this driver; actual device hook is required. Earlier candidate fixed ordering but darkened native text. Explicit Capture plus declaration and matrix restoration corrected that. Original sceneDepth rendering is retained; no gameplay/health/stamina/collision code changed.\n'
        f'- Installed proxy SHA256 {m["proxy_sha256"]}. Sources/manifest and hash-checked restore metadata refreshed. Backup: work/um-state/backups/mariosouls-ui-render-before/20261011-042922.zip.\n'
        '- Actual checks work/ui-render/check-044538: clear A:Rest at bonfire over Mario, readable bonfire menu, readable System menu masking his hat, rest/Leave/jump/menu-close resume. Final test level4 HP594 stamina95, protection flags zero, normal world collision flags. Oscar already dead, so no live NPC subtitle replay; no event/save/inventory changes made for this repair.\n'
        '- Final UITrace0, Test Mode0/Frames0, HarnessArmed0/Session0. Mario left active beside the bonfire.\n')
print('Verified build and source hashes; finalized v0.6.1 and proof screenshot.')
