import sys,json,time
from pathlib import Path
from host import Host
from transport import Session
h=Host(int(sys.argv[1]));before=h.snapshot();assert before['player']['name']=='test' and before['target']['hp']==44
s=None
try:
 s=Session();s.command(1);s.command(3,frames=30);s.command(3,frames=4,buttons=2);s.command(3,frames=45)
 print('HP',before['target']['hp'],'->',h.snapshot()['target']['hp'])
finally:
 if s:s.close();(Path(__file__).parent/'first-instrumented.json').write_text(json.dumps(s.events,indent=2))
 h.close()
