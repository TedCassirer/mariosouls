// Private visual QA. Output contains owned-ROM geometry: never commit the dump.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../../sdk/character_api.h"
#include "../../core/equipment.h"
static void check(bool ok){if(!ok){fprintf(stderr,"Equipment preview failed\n");exit(1);}}
int main(int argc,char**argv){
 if(argc!=5)return 2;
 auto module=LoadLibraryA(argv[1]);check(module!=nullptr);
 auto get=(SC_GetAPI)GetProcAddress(module,"SoulsCharacterGetAPI");check(get!=nullptr);
 auto api=get(SC_ABI_VERSION,sizeof(SC_CharacterAPI));check(api&&api->equipment&&api->attachments);
 auto d=api->describe();std::vector<unsigned char>texture(d->textureWidth*d->textureHeight*4);
 SC_Texture tex={texture.data(),(uint32_t)texture.size()};SC_Init init={sizeof(init),argv[2],argv[3],nullptr};check(api->initialize(&init,&tex));
 SC_Surface floor[2]={{{{-16000,0,-16000},{-16000,0,16000},{16000,0,16000}},0},{{{-16000,0,-16000},{16000,0,16000},{16000,0,-16000}},0}};
 api->surfaces(floor,2,0);int id=api->create(0,0,0,0);check(id>=0);
 unsigned n=d->maxTriangles;std::vector<float>p(n*9),normal(n*9),color(n*9),uv(n*6);
 SC_Mesh mesh={p.data(),normal.data(),color.data(),uv.data(),n,0};SC_State state{};SC_Input in{};in.camLookZ=1;
 for(int i=0;i<30;i++)api->step(id,&in,&state,&mesh);
 FILE*f=fopen(argv[4],"wb");check(f!=nullptr);
 unsigned count=0;for(unsigned kind:{1u,3u,4u,5u,6u,7u})count+=2*(SCEquipment::timing(kind).total+15);
 fwrite("SCPOSE1",1,8,f);fwrite(&d->textureWidth,4,1,f);fwrite(&d->textureHeight,4,1,f);fwrite(&count,4,1,f);fwrite(texture.data(),1,texture.size(),f);
 for(unsigned kind:{1u,3u,4u,5u,6u,7u})for(unsigned combo=1;combo<=2;combo++){
  auto timing=SCEquipment::timing(kind);SCEquipment::Swing swing;swing.begin(0,kind);
  for(int frame=-5;frame<timing.total+10;frame++){
   float phase=frame<0?0:swing.phase();unsigned attack=frame>=0&&frame<timing.total?combo:0;
   SC_EquipmentPose e={sizeof(e),kind,SC_EQUIPMENT_SHIELD,attack,phase,0};check(api->equipment(id,&e));
   api->nativeDamage(id,0);check(api->step(id,&in,&state,&mesh));SC_Attachments hands={sizeof(hands)};check(api->attachments(id,&hands)&&hands.valid==3);
   unsigned header[]={kind,combo,(unsigned)(frame+5),mesh.numTrianglesUsed};fwrite(header,4,4,f);fwrite(&phase,4,1,f);
   fwrite(hands.right,4,16,f);fwrite(hands.left,4,16,f);fwrite(mesh.position,4,mesh.numTrianglesUsed*9,f);fwrite(mesh.normal,4,mesh.numTrianglesUsed*9,f);fwrite(mesh.color,4,mesh.numTrianglesUsed*9,f);fwrite(mesh.uv,4,mesh.numTrianglesUsed*6,f);
   if(frame>=0&&frame<timing.total)swing.tick(false,true,0);
  }
 }
 fclose(f);api->destroy(id);api->shutdown();FreeLibrary(module);printf("Exported %u private equipment frames\n",count);
}
