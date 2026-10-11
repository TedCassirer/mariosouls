#pragma once
static uintptr_t eventFlagsSlot=0;
static size_t arenaBaseCount=0;
static bool arenaFloorPresent=false;
static std::vector<Vec> arenaFloorVertices;
static bool arenaBroken(bool& broken){
 uintptr_t flags=chain(eventFlagsSlot,{0,0});uint32_t value=0;
 // Event11810220: group1 + area181(index17) + section0 + bit220.
 if(!flags||!get(flags+0x5a18,value))return false;
 broken=(value&8)!=0;return true;
}
static void arenaLoad(){
 arenaBaseCount=mapVertices.size();arenaFloorPresent=false;arenaFloorVertices.clear();
 if(loadedMap!=1801)return;
 char path[MAX_PATH];snprintf(path,sizeof(path),"%s\\MarioSouls\\collision\\asylum-arena-floor.bin",root);
 FILE* f=fopen(path,"rb");if(!f)return;char magic[4];uint32_t n=0;
 if(fread(magic,1,4,f)==4&&!memcmp(magic,"MSC1",4)&&fread(&n,4,1,f)==1&&n>0&&n<10000){arenaFloorVertices.resize(n*3);if(fread(arenaFloorVertices.data(),sizeof(Vec),n*3,f)!=n*3)arenaFloorVertices.clear();}fclose(f);
}
static bool arenaRefresh(){
 if(loadedMap!=1801||arenaFloorVertices.empty())return false;
 bool broken;if(!arenaBroken(broken))return false;
 bool want=!broken;if(want==arenaFloorPresent)return false;
 mapVertices.resize(arenaBaseCount);if(want)mapVertices.insert(mapVertices.end(),arenaFloorVertices.begin(),arenaFloorVertices.end());
 arenaFloorPresent=want;logf("Asylum arena object floor present=%d native_broken=%d",want,broken);return true;
}
