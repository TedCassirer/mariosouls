from pathlib import Path
import json
p=Path('outputs/Combat harness/20261011-001352/interaction-regression.json');r=json.loads(p.read_text());print(r['status'],r['final']['player']['hp'],r['final']['player']['stamina'],r['final']['stats']['level'],r['final']['stats']['souls'])
r=json.loads(Path('outputs/Combat harness/20261011-001237/02-kill_reset-result.json').read_text());print('kill reset souls',r['reset_after_kill']['stats']['souls'])
