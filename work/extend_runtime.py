from pathlib import Path
p=Path(__file__).with_name('mariosouls.cpp');s=p.read_text()
s=s.replace('static uintptr_t playerSlot,cameraSlot,player,lastPlayer,posData,cam,lastMap;', 'static uintptr_t playerSlot,cameraSlot,mapSlot,player,lastPlayer,posData,cam,lastMap;\nstatic int loadedMap=-1,lastHP=0;')
s=s.replace('static bool initMario(){', '''static int currentMap(){uintptr_t p=read<uint32_t>(mapSlot);if(!p)return -1;int world=read<uint8_t>(p+0xa13),area=read<uint8_t>(p+0xa12);return world>=10&&world<=18&&area<=3?world*100+area:-1;}
static bool loadMap(){
 int map=currentMap();if(map<0)return false;if(map==loadedMap&&!mapVertices.empty())return true;
 char file[MAX_PATH];GetPrivateProfileStringA("MarioSouls","Collision","",file,sizeof(file),ini);char*slash=strrchr(file,'\\\\');if(!slash)return false;snprintf(slash+1,sizeof(file)-(slash+1-file),"m%02d_%02d_00_00.bin",map/100,map%100);
 FILE*f=fopen(file,"rb");if(!f){logf("No collision cache for map %d",map);return false;}
 char magic[4];uint32_t count=0;bool ok=fread(magic,1,4,f)==4&&memcmp(magic,"MSC1",4)==0&&fread(&count,4,1,f)==1&&count>0&&count<2000000;std::vector<Vec> data;if(ok){data.resize(count*3);ok=fread(data.data(),sizeof(Vec),count*3,f)==count*3;}fclose(f);if(!ok)return false;mapVertices.swap(data);loadedMap=map;logf("map=%d collision triangles=%u",map,count);return true;
}
static void rebuildSurfaces(){
 std::vector<SM64Surface> surfaces;surfaces.reserve(mapVertices.size()/3);bool flip=GetPrivateProfileIntA("MarioSouls","FlipWinding",1,ini)!=0;
 for(size_t i=0;i<mapVertices.size();i+=3){Vec center=(mapVertices[i]+mapVertices[i+1]+mapVertices[i+2])*(1.f/3);if(fabsf(center.x-origin.x)>110||fabsf(center.z-origin.z)>110||fabsf(center.y-origin.y)>110)continue;SM64Surface surface={};for(int v=0;v<3;v++){Vec q=(mapVertices[i+(flip?2-v:v)]-origin)*100;surface.vertices[v][0]=(int)roundf(q.x);surface.vertices[v][1]=(int)roundf(q.y);surface.vertices[v][2]=(int)roundf(q.z);}surfaces.push_back(surface);}sm_load(surfaces.data(),surfaces.size());
}
static bool initMario(){''')
start=s.index(' GetPrivateProfileStringA("MarioSouls","Collision"',s.index('static bool initMario'))
end=s.index('\n}',start)
s=s[:start]+' logf("libsm64 ready");return true;'+s[end:]
s=s.replace('if(!player||!posData||!cam||!ready)return false;', 'if(!player||!posData||!cam||!ready||!loadMap())return false;')
start=s.index(' std::vector<SM64Surface> surfaces;',s.index('static bool activate'))
end=s.index(' if(floor <',start)
s=s[:start]+''' rebuildSurfaces();float floor=sm_floor(0,100,0);logf("activate origin=%.3f %.3f %.3f map=%d floor=%.3f",origin.x,origin.y,origin.z,loadedMap,floor);
'''+s[end:]
s=s.replace('enabled=true;return true;', 'lastHP=read<int>(player+0x2d4);enabled=true;return true;')
s=s.replace('!posData||!cam||read<int>(player+0x2d4)<=0)', '!posData||!cam||currentMap()!=loadedMap||read<int>(player+0x2d4)<=0)')
s=s.replace("input.stickX=((GetAsyncKeyState('D')&0x8000)?1.f:0)-((GetAsyncKeyState('A')&0x8000)?1.f:0);", "input.stickX=((GetAsyncKeyState('A')&0x8000)?1.f:0)-((GetAsyncKeyState('D')&0x8000)?1.f:0);")
s=s.replace(' SM64MarioInputs input={};', ''' Vec host=read<Vec>(posData+0x10),expected=origin+Vec{state.position[0],state.position[1],state.position[2]}*.01f;if(dot(host-expected,host-expected)>64){logf("native warp/load detected");releaseControl();return;}
 if(fabsf(state.position[0])>4000||fabsf(state.position[2])>4000||fabsf(state.position[1])>5000){Vec shift={state.position[0],state.position[1],state.position[2]};origin=origin+shift*.01f;rebuildSurfaces();sm_position(mario,0,0,0);for(size_t i=0;i<geo.numTrianglesUsed*3;i++){gpos[i*3]-=shift.x;gpos[i*3+1]-=shift.y;gpos[i*3+2]-=shift.z;}state.position[0]=state.position[1]=state.position[2]=0;logf("collision origin rebased");}
 SM64MarioInputs input={};''')
s=s.replace('put(lastMap+0xe4,state.faceAngle)', 'put(lastMap+0xe4,state.faceAngle+3.14159265f)')
s=s.replace('ready=initMario();', 'mapSlot=scan("8B 48 04 8B 11 50 8B 42 34 FF D0 C7 46 2C 00 00 00 00 C3 8B 0D ? ? ? ? 8B 89 50 0B 00 00 32 C0 83 79 28 00",0x15);if(!mapSlot)return 0;ready=initMario();')
p.write_text(s)
# Use the actual counter hook, as the game also keeps a cached function pointer.
p=Path(__file__).with_name('startup.h');s=p.read_text()
s=s.replace('// Only the host\'s clock import is adjusted, and only while a recognized logo is drawn.', '// Counter acceleration is active only while a recognized startup logo is drawn.')
s=s.replace('if(introActive&&GetCurrentThreadId()==introThread)introOffset+=introFrequency/50;', 'if(introActive){void*frames[128];USHORT count=CaptureStackBackTrace(0,128,frames,nullptr);if(count<3)introOffset+=introFrequency/50;}')
start=s.index(' BYTE*base=')
end=s.index(' HMODULE dx=',start)
s=s[:start]+''' void*counter=(void*)QueryPerformanceCounter;if(MH_CreateHook(counter,(void*)introCounter,(void**)&realCounter)!=MH_OK){skipIntro=false;logf("startup clock hook unavailable");return;}MH_EnableHook(counter);
'''+s[end:]
p.write_text(s)
