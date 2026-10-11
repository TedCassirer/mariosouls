from pathlib import Path
w=Path(__file__).resolve().parents[1];p=w/'mario_combat_runtime.h';s=p.read_text()
start=s.index('static float combatArmorWeight(');end=s.index('static std::unordered_map',start)
s=s[:start]+'''static int marioRecoveryDelay=500;
static float combatEquipmentWeight(uintptr_t stats){
 uintptr_t repo=read<uint32_t>(0x13785b4);float total=0;
 for(int slot:{0x24c,0x250,0x254,0x258,0x26c,0x270,0x274,0x278}){
  int id=read<int>(stats+slot);if(id<0)continue;id-=id%1000;
  bool armor=slot>=0x26c;uintptr_t param=chain(repo+(armor?36:0)+12,{0,0x20});char type[32]={};
  if(!get(param+12,type)||strcmp(type,armor?"EQUIP_PARAM_PROTECTOR_ST":"EQUIP_PARAM_WEAPON_ST"))return 50.f;
  int count=read<uint16_t>(param+10);if(count<1||count>5000)return 50.f;
  int lo=0,hi=count-1;bool found=false;
  while(lo<=hi){int mid=(lo+hi)/2,key=read<int>(param+0x30+mid*12);if(key<id)lo=mid+1;else if(key>id)hi=mid-1;else{
   uint32_t offset=read<uint32_t>(param+0x34+mid*12);float weight=offset<2000000?read<float>(param+offset+(armor?0x20:0xc)):-1;
   if(!isfinite(weight)||weight<0||weight>50)return 50.f;total+=weight;found=true;break;
  }}if(!found)return 50.f;
 }return total;
}
'''+s[end:]
s=s.replace('"ArmorCostPerWeight"','"EquipmentCostPerWeight"').replace('combatArmorWeight(stats)','combatEquipmentWeight(stats)')
s=s.replace('marioRecoveryUntil=GetTickCount()+500;','marioRecoveryUntil=GetTickCount()+marioRecoveryDelay;')
s=s.replace('bool paid=marioWindow.pay(stamina,cost,power);','marioRecoveryDelay=500+(marioArmorPenalty>0?int(std::min(500.f,weight*8)):0);\n bool paid=marioWindow.pay(stamina,cost,power);')
s=s.replace('if(after==0){if(harness.armed)harnessObserve(true,controllerReader.poll(GetTickCount()).connected);','if(after==0){if(harness.armed)harnessObserve(true,pollController(GetTickCount()).connected);')
p.write_text(s)
p=w/'mariosouls.cpp';s=p.read_text().replace('harnessAfterStep(old,input);pumpAudio();combat();','harnessAfterStep(old,input);pumpAudio();if(!combatEnvironment())return;combat();');p.write_text(s)
p=w/'progression/install.py';s=p.read_text().replace("'mario_native_combat.h'","'mario_arena_floor.h','mario_native_combat.h'")
s=s.replace("p=g/'MarioSouls/install.json';", "shutil.copy2(w/'progression/asylum-arena-floor.bin',g/'MarioSouls/collision/asylum-arena-floor.bin')\np=g/'MarioSouls/install.json';")
p.write_text(s)
