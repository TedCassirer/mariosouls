"""Bounded MarioSouls combat runs. Only character 'test', map1801, ordinary bonfire resets.
No spawning, enemy writes, HP edits, save restoration, or balance changes.
"""
from pathlib import Path
import argparse,configparser,datetime,hashlib,json,math,os,secrets,sys,time,statistics,msvcrt,subprocess
from host import Host,StateError,GAME,WORK,user,k
from geometry import Geometry
from transport import Session,stop,STOP
from um.win import Drive,shot
OUT=WORK.parent/'outputs/Combat harness'
FIXTURES={
 'hollow':dict(target=1810101,model='c2500',hp=44,enemy=[-14.67,185.305,-23.074],idle=9005,player=[-14.35,185.4,-22.2],heading=.35),
 'armed':dict(target=1810103,model='c2500',hp=69,enemy=[-7.76,208.43,-5],idle=None,player=[-7.76,208.55,-1.0],heading=0),
}
RECIPES={
 'punch_in':[(4,2,0,0),(45,0,0,0)],
 'punch_out':[(4,2,0,0),(45,0,0,0)],
 'punch_wall':[(4,2,0,0),(45,0,0,0)],
 'sustained':[(45,2,0,0),(30,0,0,0)],
 'combo':[(3,2,0,0),(5,0,0,0),(3,2,0,0),(5,0,0,0),(3,2,0,0),(45,0,0,0)],
 'stomp':[(12,1,0,-.7),(8,0,0,.7),(45,0,0,0)],
 'dive':[(41,2,0,-1,35),(40,0,0,0)],
 'jump_kick':[(3,1,0,0),(6,2,0,0),(50,0,0,0)],
 'slide_kick':[(15,0,0,-1),(3,4,0,-1),(6,6,0,-1),(40,0,0,0)],
 'ground_pound':[(10,1,0,0),(8,4,0,0),(45,0,0,0)],
}
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(path,data):path.write_text(json.dumps(data,indent=2),encoding='utf-8')
class Runner:
 def __init__(self,pid,out=None):
  self.host=Host(pid,write=True);self.geo=Geometry();self.drive=Drive('DARKSOULS');self.session=None;self.started=time.monotonic();self.trace=[];self.batch=[]
  self.out=Path(out) if out else OUT/datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
  self.out.mkdir(parents=True,exist_ok=False)
  self.lock=open(GAME/'MarioSouls/harness.lock','a+b');self.lock.seek(0)
  try:msvcrt.locking(self.lock.fileno(),msvcrt.LK_NBLCK,1)
  except OSError:raise StateError('Another harness runner holds the game')
  self.build=dict(plugin=sha(GAME/'dinput8.dll'),sm64=sha(GAME/'MarioSouls/sm64.dll'),game=sha(GAME/'DARKSOULS.exe'),collision=sha(GAME/'MarioSouls/collision/m18_01_00_00.bin'))
  self.build['runner_files']={p.name:sha(p) for p in Path(__file__).parent.glob('*.py') if p.name in ['runner.py','host.py','transport.py','geometry.py']}
  cfg=configparser.ConfigParser();cfg.read(GAME/'mariosouls.ini');self.settings={section:dict(cfg[section]) for section in cfg.sections() if section!='Test'};save(self.out/'build.json',dict(timestamp=utc(),hashes=self.build,settings=self.settings,recipes=RECIPES))
  if STOP.exists():STOP.unlink()
  self.drive.focus();time.sleep(1)
 def observe(self,target=1810101,label='observe'):
  if time.monotonic()-self.started>600:raise StateError('Batch ten-minute bound reached')
  if STOP.exists():raise StateError('Cancelled by stop command')
  s=self.host.snapshot(target);s['timestamp']=utc();s['label']=label;self.trace.append(s)
  if not s['focus']:raise StateError('Game focus lost; no further input sent')
  return s
 def until(self,pred,timeout,target,label):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   s=self.observe(target,label)
   if pred(s):return s
   time.sleep(.1)
  raise StateError('Timed out: '+label)
 def native_off(self,target):
  if self.session:self.session.close();self.session=None
  self.observe(target)
  path=GAME/'mariosouls.ini';cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(path)
  cfg['Test']={k:str(v) for k,v in dict(Serial=int(cfg['Test'].get('Serial',0))+1,Mode=2,Frames=0,Buttons=0,X=0,Y=0).items()}
  tmp=path.with_name('harness-native.tmp')
  with tmp.open('w') as f:cfg.write(f)
  for i in range(40):
   try:os.replace(tmp,path);break
   except PermissionError:
    if i==39:raise
    time.sleep(.01)
  self.until(lambda s:s['player']['flags2']&0x300==0,2,target,'native-control-restored')
 def warp(self,xyz,heading,target):
  floor=self.geo.floor(xyz)
  self.trace.append(dict(timestamp=utc(),label='verified_native_player_warp',destination=xyz,floor=floor,heading=heading))
  self.host.warp(xyz,heading,target)
  return self.until(lambda s:s['native_idle'] and not s['menu'] and not s['interaction'] and abs(s['player']['position'][1]-floor)<.4,4,target,'position-settled')
 def protections(self,s):
  bad={k:v for k,v in s['protections'].items() if v}
  if bad:raise StateError('Test protections/debug overrides present: '+json.dumps(bad))
 def reset(self,fixture='hollow',test='punch_in'):
  f=FIXTURES[fixture];target=f['target'];s=self.observe(target,'reset-start');self.native_off(target)
  if s['player']['hp']<=0:
   s=self.until(lambda s:s['player']['hp']>0 and s['native_idle'] and not s['interaction'],45,target,'respawn')
  if s['menu'] or s['interaction']:raise StateError('Reset requested during a menu/interaction; close it first')
  self.warp([3.1,196,5.7],math.pi,target)
  entered=False
  for attempt in range(2):
   self.observe(target,'before-rest-key');self.drive.hold('0x51',140)
   try:self.until(lambda s:s['menu'].get('0x40')==2,5,target,'bonfire-menu');entered=True;break
   except StateError as e:
    if 'Timed out' not in str(e) or attempt:raise
  if not entered:raise StateError('Bonfire did not open')
  time.sleep(3);s=self.observe(target,'resting')
  if s['menu'].get('0x40')!=2:raise StateError('Bonfire menu changed before Leave')
  self.drive.hold('0x0D',140)
  self.until(lambda s:not s['menu'] and not s['interaction'] and s['native_idle'] and s['player']['hp']==s['stats']['hp_mod_max'],10,target,'bonfire-left')
  p=list(f['player']);heading=f['heading']
  if test=='punch_out':p=[-14.35,185.4,-20.2];heading=math.atan2(-.32,2.874)*-1
  if test=='dive':p=[-14.35,185.65,-15.0];heading=math.atan2(.32,8.074)
  if test=='slide_kick':p=[-14.35,185.4,-18.5];heading=math.atan2(.32,4.574)
  if test in ['ground_pound','stomp']:p=[-14.67,185.4,-22.65];heading=0
  if test=='punch_wall':p,heading=self.wall_position(f)
  self.warp(p,heading,target)
  s=self.until(lambda s:s['target'] and s['target']['hp']==f['hp'] and not s['menu'] and not s['interaction'] and s['native_idle'],15,target,'encounter-reacquired')
  t=s['target']
  if t['model']!=f['model'] or t['team']!=6:raise StateError('Wrong enemy model/team')
  if math.dist(t['position'],f['enemy'])>(.75 if fixture=='armed' else .35):raise StateError('Enemy moved away from fixture start')
  if f['idle'] is not None and not any(a==f['idle'] for _,a in t['tracks']):raise StateError('Target is not in expected initial behavior')
  if t['flags2']&0x8000:raise StateError('Target update disabled')
  if test=='punch_wall':
   a=s['player']['position'];b=t['position']
   if math.hypot(a[0]-b[0],a[2]-b[2])>1.3 or not self.geo.blocked([a[0],a[1]+.7,a[2]],[b[0],b[1]+.7,b[2]]):raise StateError('Inconclusive: native placement does not preserve the in-range wall fixture')
  self.protections(s)
  if s['stats']['level']!=4 or s['stats']['humanity']!=0 or s['player']['stamina']!=s['stats']['stamina_mod_max'] or s['player']['hp']!=s['stats']['hp_mod_max']:raise StateError('Unexpected level, humanity, health, or unfilled stamina')
  signature=dict(equipment=dict(s['equipment']),stats={k:v for k,v in s['stats'].items() if k not in ['souls','humanity']})
  baseline=OUT/'fixture-baseline.json'
  if baseline.exists():
   expected=json.loads(baseline.read_text())
   # This combat stage deliberately compares native hand stances. Preserve the
   # original baseline and continue checking every equipped item and attribute.
   actual_stance=signature['equipment'].pop('stance');expected['equipment'].pop('stance',None)
   if actual_stance not in (1,3) or expected!=signature:raise StateError('Player equipment/stats differ from recorded baseline; inspect before accepting a new baseline')
  else:save(baseline,signature)
  return s
 def wall_position(self,f):
  enemy=f['enemy']
  for radius in [1.0,1.15,1.25]:
   for angle in range(0,360,10):
    a=math.radians(angle);p=[enemy[0]+radius*math.sin(a),enemy[1]+.1,enemy[2]+radius*math.cos(a)]
    try:floor=self.geo.floor(p)
    except StateError:continue
    if self.geo.blocked([p[0],floor+.7,p[2]],[enemy[0],enemy[1]+.7,enemy[2]]):return p,math.atan2(p[0]-enemy[0],p[2]-enemy[2])
  raise StateError('Inconclusive: no verified safe in-range wall fixture for this hollow')
 def capture(self,name):
  try:shot(str(self.out/(name+'.png')),exe='DARKSOULS.exe',scale=.5)
  except (Exception,SystemExit) as e:self.trace.append(dict(label='capture_failed',error=str(e)))
 def run(self,name,index):
  start=time.monotonic();result=dict(test=name,index=index,timestamp=utc(),build=self.build,status='inconclusive');start_trace=len(self.trace);events=[]
  try:
   fixture='armed' if name.startswith('incoming') else 'hollow'
   before=self.reset(fixture,name);result['starting_conditions']=before
   result['suppress_outgoing']=name.startswith('incoming')
   self.session=Session(FIXTURES[fixture]['target'],suppress_outgoing=result['suppress_outgoing'],pid=self.host.pid);self.session.command(1);self.session.command(3,frames=20)
   if name in RECIPES:
    for step in RECIPES[name]:self.session.command(3,*step)
   elif name=='kill_reset':
    for attempt in range(45):
     s=self.observe(label='kill-pursuit');target=s['target']
     if not target or target['hp']<=0:break
     p=s['player']['position'];n=target['position'];dx=n[0]-p[0];dz=n[2]-p[2];distance=math.hypot(dx,dz)
     heading=before['player']['facing'];f=[-math.sin(heading),0,-math.cos(heading)];sx=-f[2]*dx+f[0]*dz;sy=-f[0]*dx-f[2]*dz;length=max(math.hypot(sx,sy),.001)
     self.session.command(3,6,2 if distance<1.2 else 0,sx/length,sy/length);self.session.command(3,6,0)
    result['killed']=self.host.snapshot()['target']['hp']==0
   elif name in ['incoming_hit','incoming_repeated','incoming_lethal','incoming_heal']:
    limit={'incoming_hit':18,'incoming_repeated':36,'incoming_lethal':66,'incoming_heal':18}[name];deadline=time.monotonic()+limit
    # Leased zero-input observation; stop on HP evidence instead of waiting for a long command.
    while time.monotonic()<deadline:
     self.session.poll()
     if STOP.exists():raise StateError('Cancelled by stop command')
     ended=next((e for e in reversed(self.session.events) if e['type']=='stopped'),None)
     if ended:
      if ended.get('reason')=='player_died':break
      raise StateError('Harness stopped: '+ended.get('reason','unknown'))
     count=sum(e['type']=='confirmed_player_hp_loss' for e in self.session.events)
     if name in ['incoming_hit','incoming_heal'] and count>=1 or name=='incoming_repeated' and count>=3:break
     time.sleep(.02)
   elif name=='cancel':
    # A command is issued without waiting for completion, then an explicit stop.
    with self.session.lock:
     self.session.serial+=1;self.session.cfg['Harness'].update(Serial=str(self.session.serial),Mode='3',Frames='150',Buttons='2',X='0',Y='0');self.session.write()
    time.sleep(.2);self.session.close()
   elif name=='lease_expiry':
    self.session.running=False;self.session.thread.join(1);time.sleep(2.4);self.session.poll()
   elif name=='focus_loss':
    # PTDE can keep foreground ownership when minimized. Switch to the existing
    # ChatGPT window instead; send no keys, clicks or text to that window.
    other=Drive('ChatGPT')
    try:
     if not other.focus():raise StateError('Inconclusive: no existing ChatGPT window available for focus test')
     time.sleep(.7)
     if self.host.focused():raise StateError('Inconclusive: operating system did not switch foreground window')
     self.session.poll()
    finally:
     self.drive.focus();time.sleep(.6);other.close()
   else:raise StateError('Unknown test '+name)
   events=self.session.close();self.session=None
   try:after=self.host.snapshot(FIXTURES[fixture]['target']);result['ending_conditions']=after
   except StateError as e:result['ending_observation_error']=str(e)
   result.update(summarize(events))
   result['status'],result['diagnostic']=classify(name,result,events)
   if name in ['cancel','lease_expiry','focus_loss'] and result.get('ending_conditions',{}).get('player',{}).get('flags2',0)&0x300:
    result['status']='failed';result['diagnostic']='Stop was recorded, but native movement/attack controls were not released.'
   if name=='incoming_heal' and result['damage_to_player']>0:
    damaged=result['ending_conditions']['player']['hp'];restored=self.reset('hollow');result['healing']={'before':damaged,'after':restored['player']['hp'],'mechanism':'ordinary bonfire rest'}
    result['status']='completed' if restored['player']['hp']>damaged else 'failed';result['diagnostic']='Measured native healing through ordinary bonfire rest.'
   if name=='kill_reset' and result.get('killed'):
    restored=self.reset();result['reset_after_kill']=restored;result['status']='completed';result['diagnostic']='Killed through ordinary attacks, then bonfire reset reacquired alive target in original behavior.'
   if name=='incoming_lethal' and any(e.get('reason')=='player_died' for e in events):
    self.drive.focus();time.sleep(1)
    restored=self.wait_respawn(45);result['respawn']=restored
   if result['status']!='completed' or index==0 or name in ['kill_reset','incoming_hit','incoming_lethal']:self.capture(f'{index:02d}-{name}')
  except (Exception,KeyboardInterrupt) as e:
   result['status']='inconclusive' if str(e).startswith('Inconclusive:') else 'failed';result['diagnostic']=f'{type(e).__name__}: {e}'
   if self.session:
    try:events=self.session.close()
    except Exception:events=self.session.events
    self.session=None
   result.update(summarize(events));self.capture(f'{index:02d}-{name}-failure')
  finally:
   result['seconds']=round(time.monotonic()-start,3);result['events_file']=f'{index:02d}-{name}-events.jsonl';result['setup_file']=f'{index:02d}-{name}-setup.json'
   (self.out/result['events_file']).write_text(''.join(json.dumps(e)+'\n' for e in events),encoding='utf-8');save(self.out/result['setup_file'],self.trace[start_trace:]);save(self.out/f'{index:02d}-{name}-result.json',result);self.batch.append(result);self.report()
  print(name,result['status'],result.get('damage_to_target'),result.get('diagnostic'),flush=True)
  return result
 def wait_respawn(self,timeout):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   try:
    s=self.observe(1810103,'death-respawn')
    if s['player']['hp']>0 and s['native_idle'] and not s['menu'] and not s['interaction'] and not s['player']['flags2']&0x300:return s
   except StateError as e:
    if 'Loading' not in str(e) and 'reference' not in str(e):raise
    self.trace.append(dict(timestamp=utc(),label='death-respawn-loading',diagnostic=str(e)))
   time.sleep(.2)
  raise StateError('Death observed but respawn not verified within bound')
 def report(self):
  save(self.out/'results.json',self.batch)
  lines=['MarioSouls combat harness results','',f'Plugin SHA256: {self.build["plugin"]}','', '| Test | Status | Enemy HP loss | Player HP loss | Confirmed enemy hits | Stamina charged | Note |','|---|---|---:|---:|---:|---:|---|']
  for r in self.batch:lines.append(f'| {r["test"]} | {r["status"]} | {r.get("damage_to_target",0)} | {r.get("damage_to_player",0)} | {r.get("confirmed_hits",0)} | {r.get("stamina_charged",0)} | {r.get("diagnostic","").replace("|","/")} |')
  for name in sorted(set(r['test'] for r in self.batch)):
   group=[r for r in self.batch if r['test']==name and r['status']=='completed']
   if len(group)>1:
    vals=[r['damage_to_target'] for r in group];lines+=['',f'{name}: {len(vals)} completed repeats; measured damage {vals}, mean {statistics.mean(vals):.2f}, range {min(vals)}–{max(vals)}.']
  lines+=['','HP-loss events are observations of actual health changes. Native call return values are logged separately and do not count as hits. Stamina charged sums paid attack events; sampled decreases remain in the raw results because regeneration suppression can inflate them.','The fixture leaves AI and defenses active; position/animation traces expose movement variation. Build and settings identify the experimental combat rules used.']
  (self.out/'Report.md').write_text('\n'.join(lines),encoding='utf-8')
 def close(self):
  if self.session:self.session.close()
  self.drive.close();self.host.close()
  self.lock.seek(0);msvcrt.locking(self.lock.fileno(),msvcrt.LK_UNLCK,1);self.lock.close()
def summarize(events):
 hits=[e for e in events if e['type']=='confirmed_target_hp_loss'];incoming=[e for e in events if e['type']=='confirmed_player_hp_loss'];obs=[e for e in events if e['type']=='observation'];stamina=sum(max(0,a['stamina']-b['stamina']) for a,b in zip(obs,obs[1:]))
 return dict(damage_to_target=sum(e['damage'] for e in hits),damage_to_player=sum(e['damage'] for e in incoming),confirmed_hits=len(hits),confirmed_player_hits=len(incoming),hits=hits,incoming=incoming,stamina_decreases=stamina,stamina_charged=sum(e.get('cost',0) for e in events if e['type']=='mario_attack'),attacks=[e for e in events if e['type']=='mario_attack'],actions=sorted({e['action'] for e in events if e['type']=='input_frame'}),animations=sorted({e['animation'] for e in events if e['type']=='input_frame'}),attempts=sum(e['type']=='hit_attempt' for e in events),native_returns=[e for e in events if e['type']=='native_call_return'])
def classify(name,r,events):
 stops=[e['reason'] for e in events if e['type']=='stopped'];damage=r['damage_to_target'];hits=r['confirmed_hits']
 if name in ['cancel','lease_expiry','focus_loss']:
  reason={'cancel':'cancelled','lease_expiry':'lease_expired','focus_loss':'focus_lost'}[name]
  return ('completed','Synthetic input stopped: '+reason) if reason in stops else ('failed','Expected stop reason was not observed')
 if name.startswith('incoming'):
  required={'incoming_hit':1,'incoming_repeated':3,'incoming_lethal':1,'incoming_heal':1}[name]
  if name=='incoming_lethal':return ('completed','Native death observed; respawn verification follows.') if 'player_died' in stops else ('inconclusive','Lethal hit not observed within the test bound.')
  if name=='incoming_hit' and r['confirmed_player_hits']!=1:return 'inconclusive','Exactly one HP decrease was not isolated; inspect individual hit timings.'
  return ('completed','Observed player HP loss from ordinary enemy attacks, with protections off.') if r['confirmed_player_hits']>=required else ('inconclusive','Enemy did not deliver enough hits within bound.')
 if any(reason!='cancelled' for reason in stops):return 'failed','Unexpected stop: '+','.join(stops)
 if name in ['punch_out','punch_wall']:return ('completed','No target HP loss.') if damage==0 else ('failed','Target took damage in a negative-control test.')
 if name=='punch_in':return ('completed','One confirmed HP decrease.') if hits==1 else ('inconclusive' if hits==0 else 'failed','Expected exactly one HP decrease; examine target motion and trace.')
 if name=='sustained':
  if hits==0:return 'inconclusive','No contact damage occurred; repeated-hit suppression was not exercised.'
  return ('completed','One confirmed hit during sustained input; see target-motion trace.') if hits==1 else ('failed','Repeated confirmed hits during sustained input; inspect attack identities before balance changes.')
 if name=='kill_reset':return ('completed','Kill observed; reset verification follows.') if r.get('killed') else ('inconclusive','Target was not killed within the bounded pursuit.')
 if name=='combo':
  anim=set(r['animations']);identified=bool(anim&{0x67,0x69}) and bool(anim&{0x68,0x6a}) and 0x66 in anim
  return ('completed','First punch, second punch, and kick animations identified; HP losses measured separately.') if identified else ('inconclusive','Full punch–punch–kick sequence was not identified; do not use this run as combo balance evidence.')
 expected={'dive':0x0188088a,'ground_pound':0x008008a9,'jump_kick':0x018008ac,'slide_kick':0x018008aa}
 if name in expected:
  if expected[name] not in r['actions']:return 'inconclusive','Requested attack action did not occur.'
  own={'dive':990054,'ground_pound':990057,'jump_kick':990053,'slide_kick':990055}
  attempts={e['attempt_id'] for e in r['native_returns'] if e.get('behavior')==own[name]}
  attributed=sum(e['damage'] for e in r['hits'] if e.get('attempt_id') in attempts)
  return ('completed',f'Requested attack identified; {attributed} HP loss linked to its native hit attempt.') if attributed>0 else ('inconclusive','Attack action occurred, but no HP loss was linked to that attack.')
 if name=='stomp':
  attempts={e['attempt_id'] for e in r['native_returns'] if e.get('behavior')==990056}
  attributed=sum(e['damage'] for e in r['hits'] if e.get('attempt_id') in attempts)
  return ('completed',f'Confirmed stomp-only native record caused {attributed} HP loss.') if attributed>0 else ('inconclusive','Jump did not produce an attributed stomp hit.')
 return 'completed','Input recipe executed; inspect event trace.'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['observe','reset','batch','sequence','stop']);p.add_argument('--pid',type=int);p.add_argument('--tests',default='punch_in,punch_in,punch_in,punch_out,sustained');p.add_argument('--sequence',help='JSON list of frames/buttons/x/y objects, at most 90 seconds');p.add_argument('--output');args=p.parse_args()
 if args.command=='stop':stop();print('Stop requested. Synthetic input lease also expires within two seconds.');return
 if not args.pid:raise SystemExit('--pid is required; use the currently running DARKSOULS process')
 names=args.tests.split(',')
 if args.command=='sequence':
  if not args.sequence:raise StateError('--sequence JSON path is required')
  steps=json.loads(Path(args.sequence).read_text());recipe=[]
  if not isinstance(steps,list) or not 1<=len(steps)<=60:raise StateError('Sequence must have 1–60 steps')
  for step in steps:
   f,b,x,y=(step.get(k,0) for k in ['frames','buttons','x','y'])
   if type(f)!=int or type(b)!=int or not 1<=f<=150 or not 0<=b<=7 or not math.isfinite(x) or not math.isfinite(y) or abs(x)>1 or abs(y)>1:raise StateError('Invalid or unbounded sequence step')
   recipe.append((f,b,x,y))
  if sum(s[0] for s in recipe)>2700:raise StateError('Sequence exceeds 90 seconds')
  RECIPES['custom']=recipe;names=['custom']
 if not 1<=len(names)<=12 or any(n not in set(RECIPES)|{'kill_reset','incoming_hit','incoming_repeated','incoming_heal','incoming_lethal','cancel','lease_expiry','focus_loss'} for n in names):raise StateError('Unknown test or batch outside 1–12 tests')
 if args.command=='observe':
  h=Host(args.pid)
  try:print(json.dumps(h.snapshot(),indent=2))
  finally:h.close()
  return
 r=Runner(args.pid,args.output)
 try:
  if args.command=='reset':save(r.out/'reset.json',r.reset());print('Encounter reset and verified.')
  else:
   for i,name in enumerate(names):
    result=r.run(name,i)
    if result['status']=='failed':break
 finally:r.close()
 print('Results:',r.out,flush=True)
 return 1 if any(x['status']=='failed' for x in r.batch) else 2 if any(x['status']=='inconclusive' for x in r.batch) else 0
if __name__=='__main__':sys.exit(main())
