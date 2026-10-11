#pragma once
static const char* attackName(int id){return id>=0&&profile&&id<int(profile->attackCount)?profile->attacks[id].name:"none";}
static const SC_Attack* attackDefinition(int id){return id>=0&&profile&&id<int(profile->attackCount)?&profile->attacks[id]:nullptr;}
static bool automaticContact(Vec delta,const SC_Attack&a){return delta.y>a.automaticMinHeightCm*.01f&&delta.y<a.automaticMaxHeightCm*.01f&&delta.x*delta.x+delta.z*delta.z<a.automaticRadiusCm*a.automaticRadiusCm*.0001f;}
static void adapterLog(const char*message){logf("ADAPTER %s",message?message:"");}
static bool initCharacter(){
 char id[48]={},file[MAX_PATH]={},defaultPath[MAX_PATH]={},section[80]={},asset[MAX_PATH]={},runtime[MAX_PATH]={};
 // The legacy configuration file is retained for existing controls and test tools.
 GetPrivateProfileStringA("Framework","Character","mario64",id,sizeof(id),ini);
 if(!SC::identifier(id,sizeof(id))){logf("Invalid character identifier");return false;}
 snprintf(defaultPath,sizeof(defaultPath),"%s\\SoulsCharacterFramework\\characters\\%s\\character.dll",root,id);
 GetPrivateProfileStringA("Framework","Adapter",defaultPath,file,sizeof(file),ini);
 adapterModule=LoadLibraryExA(file,nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!adapterModule){logf("Character adapter missing: %s error=%lu",file,GetLastError());return false;}
 auto entry=(SC_GetAPI)GetProcAddress(adapterModule,"SoulsCharacterGetAPI");
 guest=entry?entry(SC_ABI_VERSION,sizeof(SC_CharacterAPI)):nullptr;
 if(!SC::api(guest)||!SC::descriptor(guest->describe())){logf("Character ABI/profile rejected");FreeLibrary(adapterModule);adapterModule=nullptr;guest=nullptr;return false;}
 profile=guest->describe();if(strcmp(profile->id,id)){logf("Character identity mismatch");FreeLibrary(adapterModule);adapterModule=nullptr;guest=nullptr;profile=nullptr;return false;}
 gpos.resize(profile->maxTriangles*9);gnormal.resize(profile->maxTriangles*9);gcolor.resize(profile->maxTriangles*9);guv.resize(profile->maxTriangles*6);
 geo={gpos.data(),gnormal.data(),gcolor.data(),guv.data(),profile->maxTriangles,0};texture.resize(profile->textureWidth*profile->textureHeight*4);
 snprintf(section,sizeof(section),"Character.%s",id);
 GetPrivateProfileStringA("MarioSouls","ROM","",defaultPath,sizeof(defaultPath),ini);GetPrivateProfileStringA(section,"Asset",defaultPath,asset,sizeof(asset),ini);
 GetPrivateProfileStringA("MarioSouls","Library","",defaultPath,sizeof(defaultPath),ini);GetPrivateProfileStringA(section,"Runtime",defaultPath,runtime,sizeof(runtime),ini);
 SC_Init init={sizeof(SC_Init),asset,runtime,adapterLog};SC_Texture atlas={texture.data(),uint32_t(texture.size())};
 if(!guest->initialize(&init,&atlas)){logf("Character initialization failed");guest->shutdown();FreeLibrary(adapterModule);adapterModule=nullptr;guest=nullptr;profile=nullptr;return false;}
 const unsigned char prefix[]={0xd9,0x44,0x24,0x18,0x8b,0x44,0x24,0x0c,0x8b,0x4c,0x24,0x04};unsigned char actual[sizeof(prefix)]={};
 if(get(uintptr_t(0xd65dc0),actual)&&memcmp(actual,prefix,sizeof(prefix))==0)nativeDamage=(DamageFn)0xd65dc0;
 combatSettings();
 if(profile->capabilities&SC_CAP_AUDIO){
  WAVEFORMATEX format={};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=profile->audioRate;format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=format.nSamplesPerSec*4;
  if(waveOutOpen(&audioOut,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)==MMSYSERR_NOERROR){waveOutSetVolume(audioOut,0x60006000);for(int i=0;i<4;i++){audioHeaders[i].lpData=(LPSTR)audioBuffers[i];audioHeaders[i].dwBufferLength=sizeof(audioBuffers[i]);waveOutPrepareHeader(audioOut,&audioHeaders[i],sizeof(WAVEHDR));}}else audioOut=nullptr;
 }
 logf("Framework ABI=%u character=%s attacks=%u atlas=%ux%u ready",SC_ABI_VERSION,profile->id,profile->attackCount,profile->textureWidth,profile->textureHeight);return true;
}
