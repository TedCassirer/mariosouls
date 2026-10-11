"""Fit original Fire Flower ready/windup/throw/recovery poses to the owned skeleton."""
from pathlib import Path
import argparse, math
from author_equipment import idle, fit, np

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',required=True);p.add_argument('--output',required=True)
    args=p.parse_args();base=idle(args.rom)
    seed=np.array([-6463,-7897,-30470,37,-26207,7206,26008])*math.pi/32768
    ready=([145,195,90],[0,0,1],0,0)
    keys=[ready,([170,335,0],[.2,0,.98],-22,-6),
          ([80,260,190],[0,0,1],12,8),([135,215,125],[0,0,1],5,3),ready]
    rows=[]
    for key in keys:
        angles,seed,error=fit(base,key,seed);rows.append(angles)
        print(f'Glove fitting error: {error:.2f} cm')
    rows[-1]=rows[0].copy()
    Path(args.output).write_text('/* Original authored flower poses; author_flower.py. */\n'
        +'static const s16 scFlowerKeys[5][12]={\n'
        +'\n'.join(' {'+','.join(map(str,r))+'},' for r in rows)+'\n};\n')

if __name__=='__main__':main()
