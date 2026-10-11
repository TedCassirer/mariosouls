"""Bounded controls for the dedicated Mario QA save; foreground and identity required."""
import argparse
import configparser
import os
from host import Host,GAME

def command(pid,mode=0,frames=0,buttons=0,x=0,y=0):
 h=Host(pid)
 try:
  state=h.snapshot()
  if not state['focus'] or state['player']['hp']<=0:raise RuntimeError('Requires focused, living Mario QA')
  if mode not in range(5) or not 0<=frames<=150 or not 0<=buttons<=15 or x not in (-1,0,1) or y not in (-1,0,1):raise ValueError('Invalid bounded test command')
  path=GAME/'mariosouls.ini';cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(path)
  if 'Test' not in cfg:cfg['Test']={}
  # The retained legacy wire format has reversed axes; this public helper uses
  # the SDK convention (right/forward positive).
  cfg['Test'].update(Serial=str(int(cfg['Test'].get('Serial',0))+1),Mode=str(mode),Frames=str(frames),Buttons=str(buttons),X=str(-x),Y=str(-y))
  tmp=path.with_suffix('.tmp')
  with tmp.open('w') as stream:cfg.write(stream)
  os.replace(tmp,path)
 finally:h.close()

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('pid',type=int)
 for key,default in [('mode',0),('frames',0),('buttons',0),('x',0),('y',0)]:p.add_argument('--'+key,type=int,default=default)
 command(**vars(p.parse_args()))
