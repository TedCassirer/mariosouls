"""Focused body/fall regression. Inputs and warps require living Mario QA."""
from pathlib import Path
import sys,time,json,math
from host import Host,GAME
from bridge import command
from um.win import Drive,shot

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'work/body-qa'
pid=int(sys.argv[1]);stage=sys.argv[2]
h=Host(pid,write=True);d=Drive('DARKSOULS');rows=[]
logstart=(GAME/'mariosouls.log').stat().st_size

def sample(label,target=1810101):
 s=h.snapshot(target);assert s['focus'] and s['player']['hp']>0
 assert not any(s['protections'].values()),s['protections']
 ph=h.chain(int(s['player']['reference'],16),0x28,0x1c)
 shapes=[h.u(ph+0x3c),h.u(ph+0x40),h.chain(ph,0x48,0x1c)]
 s['body']=[dict(radius=h.f(a+0x10),top=h.f(a+0x24),bottom=h.f(a+0x34)) for a in shapes]
 s['body_height']=h.f(ph+0x204);s['body_radius']=h.f(ph+0x208)
 s['label']=label;rows.append(s);return s

def observe(label,seconds,target=1810101):
 end=time.monotonic()+seconds;out=[]
 while time.monotonic()<end:out.append(sample(label,target));time.sleep(.04)
 return out

def key(vk,ms=140):
 assert h.focused();d.cmd('hold '+vk+' '+str(ms),retry=False)

def at(xyz,yaw=0):
 command(pid,mode=2);observe('native_before_warp',.6);h.warp(xyz,yaw);key('0x4F');observe('settle',.7)
 command(pid,mode=1);observe('activated',.8)

try:
 sample('start')
 if stage=='cycles':
  for n in range(3):
   command(pid,mode=2);s=observe('native_'+str(n),.6)[-1]
   assert abs(s['body'][0]['radius']-.4)<.001 and abs(s['body'][2]['radius']-.45)<.001
   assert abs(s['body_height']-1.5)<.001
   command(pid,mode=1);s=observe('mario_'+str(n),.6)[-1]
   assert abs(s['body'][0]['radius']-.22)<.001 and abs(s['body'][2]['radius']-.24)<.001
   assert abs(s['body_height']-1.05)<.001
  y=s['player']['position'][1];sp=s['player']['stamina']
  command(pid,frames=8,buttons=1);jump=observe('jump',2.8)
  assert 2.3<max(s['player']['position'][1] for s in jump)-y<2.6
  assert min(s['player']['stamina'] for s in jump)>=sp
 elif stage=='rest':
  at([3.1,196.5,6.55],math.pi)
  command(pid,mode=4);rest=observe('rest',3)
  assert any(s['menu'] for s in rest)
  key('0x0D');leave=observe('leave',3)
  assert not leave[-1]['menu'] and not leave[-1]['interaction']
  assert all(abs(s['body'][2]['radius']-.24)<.001 for s in rest+leave)
  command(pid,frames=8,buttons=1);observe('post_rest_jump',2.5)
 elif stage=='enemy':
  at([-14.668,185.8,-21.4]);command(pid,frames=75,y=1);observe('enemy_push',3.3);command(pid);observe('enemy_release',.6)
 elif stage=='door':
  at([3.2,198.2,-1.8]);command(pid,frames=75,y=1);observe('door_push',3.3);command(pid);observe('door_release',.6)
 elif stage=='incoming':
  try:
   at([-7.76,208.6,-3.8]);end=time.monotonic()+5
   while time.monotonic()<end:
    s=sample('enemy_melee',1810103)
    if s['player']['hp']<200:break
    time.sleep(.04)
  finally:
   # Do not leave a living QA character unattended beside an attacking enemy.
   command(pid,mode=2);observe('retreat_release',.6)
   h.warp([3.1,196.5,6.55],math.pi);observe('retreated',.6)
 elif stage=='drop':
  # Existing guarded transport owns heartbeat/target identity; does not change HP.
  sys.path.insert(0,str(ROOT/'work/combat-harness'))
  from transport import Session
  at([3.1,196.5,5.7],math.pi)
  height=int(sys.argv[3]);session=Session(1810960,suppress_outgoing=True,pid=pid)
  try:
   session.command(3,20);session.command(5,frames=height)
   end=time.monotonic()+7
   while time.monotonic()<end:session.poll();sample('drop',1810960);time.sleep(.04)
  finally:
   events=session.close();(OUT/('drop-'+str(height)+'-events.json')).write_text(json.dumps(events,indent=2))
 else:raise ValueError(stage)
 shot(str(OUT/(stage+'.png')),exe='DARKSOULS.exe',scale=.5)
 print(json.dumps({'stage':stage,'first_hp':rows[0]['player']['hp'],'last_hp':rows[-1]['player']['hp'],'last_position':rows[-1]['player']['position'],'last_body':rows[-1]['body']}))
finally:
 (OUT/(stage+('-'+sys.argv[3] if len(sys.argv)>3 else '')+'.json')).write_text(json.dumps(rows,indent=2))
 (OUT/(stage+('-'+sys.argv[3] if len(sys.argv)>3 else '')+'.log')).write_bytes((GAME/'mariosouls.log').read_bytes()[logstart:])
 try:command(pid)
 finally:h.close();d.close()
