"""Generate our own parameter records from the documented binary schema."""
import sys,json
from pathlib import Path
w=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(w/'soulstruct-source/src'),str(w/'python-deps')]
from soulstruct.darksouls1ptde.params.paramdef.ATK_PARAM_ST import ATK_PARAM_ST
from soulstruct.darksouls1ptde.params.paramdef.BEHAVIOR_PARAM_ST import BEHAVIOR_PARAM_ST
atk=ATK_PARAM_ST(PhysicalAttackPowerPercentage=0,MagicAttackPowerPercentage=0,FireAttackPowerPercentage=0,LightningAttackPowerPercentage=0,StaminaAttackPowerPercentage=0,PhysicalAttackPower=100,StaminaAttackPower=16,PoiseAttackPower=5,GuardAttackPower=100,GuardBreakPower=100,ImpactLevel=1,AttackAttribute=2,ElementAttribute=1,KnockbackDistance=.3,Hitbox0Radius=.5)
behavior=BEHAVIOR_PARAM_ST(ReferenceID=990050,Category=0)
lines=['// Generated from new Mario-owned records. No original game rows included.']
for name,row in [('marioAttackTemplate',atk),('marioBehaviorTemplate',behavior)]:
 b=bytes(row);lines.append(f'static const unsigned char {name}[{len(b)}]={{'+','.join(f'0x{x:02x}' for x in b)+'};')
 print(name,len(b))
 if name=='marioAttackTemplate':
  offsets={}
  for field in ['PhysicalAttackPower','StaminaAttackPower','PoiseAttackPower','ImpactLevel']:
   old=getattr(row,field);setattr(row,field,old+1);diff=[i for i,(a,c) in enumerate(zip(b,bytes(row))) if a!=c];setattr(row,field,old);offsets[field]=diff
  print(offsets)
(w/'mario_param_templates.h').write_text('\n'.join(lines)+'\n')
