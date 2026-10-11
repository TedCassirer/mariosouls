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
    int run=atoi(argv[3]);
    for(int i=0;i<run+35;i++){
      in.stickY=(i<run || i>=run+2 && i<run+30)?-1:0;
      in.buttonA=(i>=run+2 && i<run+22);in.buttonB=(i>=run+24 && i<run+30);
      tick(id,&in,&s,&geo);
      if(i>=run-1)printf("%d action=%08lx speed=%.2f z=%.1f y=%.1f\n",i,(unsigned long)s.action,s.forwardVelocity,s.position[2],s.position[1]);
    }
    done();return 0;
}

