#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libsm64/src/libsm64.h"

int main(int argc, char **argv) {
    if(argc < 3) { fprintf(stderr,"usage: smoke sm64.dll owned-rom.z64 [collision-cache]\n"); return 2; }
    HMODULE m=LoadLibraryA(argv[1]); if(!m) return 3;
    void (*init)(const uint8_t*,uint8_t*)=(void*)GetProcAddress(m,"sm64_global_init");
    void (*audio_init)(const uint8_t*)=(void*)GetProcAddress(m,"sm64_audio_init");
    uint32_t (*audio_tick)(uint32_t,uint32_t,int16_t*)=(void*)GetProcAddress(m,"sm64_audio_tick");
    void (*surfaces)(const struct SM64Surface*,uint32_t)=(void*)GetProcAddress(m,"sm64_static_surfaces_load");
    int32_t (*create)(float,float,float)=(void*)GetProcAddress(m,"sm64_mario_create");
    void (*tick)(int32_t,const struct SM64MarioInputs*,struct SM64MarioState*,struct SM64MarioGeometryBuffers*)=(void*)GetProcAddress(m,"sm64_mario_tick");
    void (*done)(void)=(void*)GetProcAddress(m,"sm64_global_terminate");
    if(!init || !surfaces || !create || !tick || !done) return 4;
    FILE *f=fopen(argv[2],"rb"); if(!f) return 5;
    fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
    if(size!=8388608) return 6;
    uint8_t *rom=malloc(size),*tex=malloc(SM64_TEXTURE_WIDTH*SM64_TEXTURE_HEIGHT*4);
    if(fread(rom,1,size,f)!=(size_t)size) return 7; fclose(f);
    init(rom,tex);audio_init(rom);
    struct SM64Surface floor[2]={
        {0,0,0,{{-16000,0,-16000},{-16000,0,16000},{16000,0,16000}}},
        {0,0,0,{{-16000,0,-16000},{16000,0,16000},{16000,0,-16000}}}
    };
    surfaces(floor,2);int id=create(0,30,0);if(id<0) return 8;
    float p[SM64_GEO_MAX_TRIANGLES*9],n[SM64_GEO_MAX_TRIANGLES*9],c[SM64_GEO_MAX_TRIANGLES*9],uv[SM64_GEO_MAX_TRIANGLES*6];
    struct SM64MarioGeometryBuffers geo={p,n,c,uv,0};
    struct SM64MarioState s={0};struct SM64MarioInputs in={0,1,0,0,0,0,0};
    for(int i=0;i<30;i++)tick(id,&in,&s,&geo);
    float ground=s.position[1],top=ground;
    for(int i=0;i<50;i++){in.buttonA=(i<8);tick(id,&in,&s,&geo);if(s.position[1]>top)top=s.position[1];}
    printf("id=%d triangles=%u jump_height=%.1f landing_height=%.1f\n",id,geo.numTrianglesUsed,top-ground,s.position[1]);
    int failed=(top-ground<50 || fabsf(s.position[1]-ground)>5 || !geo.numTrianglesUsed);
    for(int i=0;i<45;i++){in.stickY=-1;tick(id,&in,&s,&geo);}
    printf("run: speed=%.2f position=(%.1f %.1f %.1f)\n",s.forwardVelocity,s.position[0],s.position[1],s.position[2]);
    failed |= s.forwardVelocity<10;
    in.buttonZ=1;tick(id,&in,&s,&geo);in.buttonA=1;tick(id,&in,&s,&geo);
    printf("crouch+jump while running: action=0x%08lx velocity_y=%.1f\n",(unsigned long)s.action,s.velocity[1]);
    failed |= s.action!=0x03000888;
    if(argc>3){
        float (*findfloor)(float,float,float)=(void*)GetProcAddress(m,"sm64_surface_find_floor_height");
        FILE *cf=fopen(argv[3],"rb"); if(!cf)return 9;char magic[4];uint32_t count;fread(magic,1,4,cf);fread(&count,4,1,cf);
        if(memcmp(magic,"MSC1",4)||count>2000000)return 10;
        float *tri=malloc(count*9*sizeof(float));fread(tri,sizeof(float),count*9,cf);fclose(cf);
        struct SM64Surface *sf=calloc(count,sizeof(*sf));float org[3]={-12.29466f,184.73906f,-44.59058f};
        for(int flip=0;flip<2;flip++){
            for(uint32_t i=0;i<count;i++)for(int j=0;j<3;j++)for(int k=0;k<3;k++)sf[i].vertices[j][k]=(int)roundf((tri[i*9+(flip?2-j:j)*3+k]-org[k])*100.f);
            surfaces(sf,count);printf("Asylum spawn floor, winding %d: %.3f\n",flip,findfloor(0,100,0));
        }
        free(tri);free(sf);
    }
    void (*pose)(int32_t,float,float,float,float,int32_t,int32_t,struct SM64MarioState*,struct SM64MarioGeometryBuffers*)=(void*)GetProcAddress(m,"sm64_mario_pose_tick");if(!pose)return 11;
    int poses[]={5,13,0x60,0x85,0xc3};
    for(int a=0;a<5;a++){int first=-1,last=-1;for(int frame=0;frame<45;frame++){
        pose(id,100,5000,200,.75f,poses[a],-1,&s,&geo);if(frame==0)first=s.animFrame;last=s.animFrame;
        failed|=fabsf(s.position[1]-5000)>.001f||s.animID!=poses[a]||!geo.numTrianglesUsed;
        for(unsigned i=0;i<geo.numTrianglesUsed*9;i++)failed|=!isfinite(p[i]);
    }printf("host pose anim=%d frames=%d->%d triangles=%u fixed_height=%.1f\n",poses[a],first,last,geo.numTrianglesUsed,s.position[1]);}
    surfaces(floor,2);void (*setpos)(int32_t,float,float,float)=(void*)GetProcAddress(m,"sm64_set_mario_position");void (*setaction)(int32_t,uint32_t)=(void*)GetProcAddress(m,"sm64_set_mario_action");setpos(id,0,10,0);setaction(id,0x0c400201);memset(&in,0,sizeof(in));in.camLookZ=1;
    void (*hostpose)(int32_t,float,float,float,float,int32_t,float,int32_t,struct SM64MarioState*,struct SM64MarioGeometryBuffers*)=(void*)GetProcAddress(m,"sm64_mario_host_pose_tick");
    void (*resume)(int32_t,float,float,float,float)=(void*)GetProcAddress(m,"sm64_mario_host_resume");
    if(!hostpose||!resume)return 12;
    for(int k=0;k<6;k++)for(int frame=0;frame<40;frame++){
        hostpose(id,0,0,0,0,k<3?0x30:0x84,frame/39.f,k<3?4:1,&s,&geo);
        for(unsigned i=0;i<geo.numTrianglesUsed*9;i++)failed|=!isfinite(p[i]);
        failed|=geo.numTrianglesUsed!=752||fabsf(s.position[1])>.01f;
    }
    resume(id,0,0,0,0);memset(&in,0,sizeof(in));in.camLookZ=1;
    tick(id,&in,&s,&geo);in.buttonA=1;tick(id,&in,&s,&geo);
    failed|=!(s.action&0x800);printf("host actions same-instance resume jump action=%08lx\n",(unsigned long)s.action);
    int16_t audio[2176];for(int i=0;i<5000;i++){tick(id,&in,&s,&geo);audio_tick(0,1100,audio);}in.buttonA=1;tick(id,&in,&s,&geo);printf("sleep/wake regression: action=%08lx\n",(unsigned long)s.action);
    done();free(rom);free(tex);FreeLibrary(m);
    puts(failed?"FAIL":"PASS: ROM init, mesh, jump, run, long jump");return failed;
}
