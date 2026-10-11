#pragma once
// Neutral, bounded triangle-soup + sampled skeletal animation asset format.
// Game archive parsing remains in each character's offline asset recipe.
#include "../sdk/character_api.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace SCMesh {
struct Vertex {float p[3],n[3],uv[2],color[3];uint16_t bone[4];float weight[4];uint32_t group;};
static_assert(sizeof(Vertex)==72,"SCMESH01 vertex layout");
struct Clip {std::string name;uint32_t frames=0,loop=0;std::vector<float> matrices;};
class Pack {
public:
 uint32_t width=0,height=0,bones=0;std::vector<Vertex> vertices;std::vector<uint8_t> texture;std::vector<Clip> clips;
 void clear(){width=height=bones=0;vertices.clear();texture.clear();clips.clear();}
 bool load(const char*path){
  clear();if(!path)return false;FILE*f=fopen(path,"rb");if(!f)return false;
  fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);if(length<28||length>64*1024*1024){fclose(f);return false;}
  std::vector<uint8_t> bytes(length);bool ok=fread(bytes.data(),1,length,f)==size_t(length);fclose(f);if(!ok)return false;
  size_t at=0;auto read=[&](void*p,size_t n){if(n>bytes.size()-at)return false;memcpy(p,bytes.data()+at,n);at+=n;return true;};
  char magic[8];uint32_t h[5];if(!read(magic,8)||memcmp(magic,"SCMESH01",8)||!read(h,20))return false;
  if(!h[0]||h[0]>2048||!h[1]||h[1]>2048||!h[2]||h[2]>SC_MAX_TRIANGLES*3||h[2]%3||!h[3]||h[3]>256||!h[4]||h[4]>128)return false;
  width=h[0];height=h[1];bones=h[3];texture.resize(width*height*4);vertices.resize(h[2]);
  if(!read(texture.data(),texture.size())||!read(vertices.data(),vertices.size()*sizeof(Vertex))){clear();return false;}
  for(size_t i=0;i<vertices.size();i++){
   const auto&v=vertices[i];float total=0;
   for(float x:v.p)if(!std::isfinite(x)||fabsf(x)>10000){clear();return false;}
   for(float x:v.n)if(!std::isfinite(x)||fabsf(x)>2){clear();return false;}
   for(float x:v.uv)if(!std::isfinite(x)||x<0||x>1){clear();return false;}
   for(float x:v.color)if(!std::isfinite(x)||x<0||x>1){clear();return false;}
   for(int k=0;k<4;k++){if(v.bone[k]>=bones||!std::isfinite(v.weight[k])||v.weight[k]<0||v.weight[k]>1){clear();return false;}total+=v.weight[k];}
   if(fabsf(total-1)>.001f||!v.group||(i%3&&v.group!=vertices[i-i%3].group)){clear();return false;}
  }
  for(unsigned i=0;i<h[4];i++){
   char name[32];Clip c;if(!read(name,32)||!memchr(name,0,32)||!name[0]||!read(&c.frames,4)||!read(&c.loop,4)||!c.frames||c.frames>601||c.loop>1){clear();return false;}
   c.name=name;for(const auto&prior:clips)if(prior.name==c.name){clear();return false;}
   size_t size=size_t(c.frames)*bones*12;if(size*4>bytes.size()-at){clear();return false;}c.matrices.resize(size);
   if(!read(c.matrices.data(),size*4)){clear();return false;}
   for(float x:c.matrices)if(!std::isfinite(x)||fabsf(x)>100000){clear();return false;}
   clips.push_back(std::move(c));
  }
  if(at!=bytes.size()){clear();return false;}return true;
 }
 int clip(const char*name)const{for(size_t i=0;i<clips.size();i++)if(clips[i].name==name)return int(i);return -1;}
 bool sample(int index,float frame,std::vector<float>&out)const{
  if(index<0||size_t(index)>=clips.size()||!std::isfinite(frame))return false;const auto&c=clips[index];
  frame=std::max(0.f,frame);if(c.loop&&c.frames>1)frame=fmodf(frame,float(c.frames-1));else frame=std::min(frame,float(c.frames-1));
  unsigned a=unsigned(frame),b=std::min(a+1,c.frames-1);float t=frame-a;size_t n=bones*12;out.resize(n);
  for(size_t i=0;i<n;i++)out[i]=c.matrices[a*n+i]*(1-t)+c.matrices[b*n+i]*t;return true;
 }
 bool draw(const std::vector<float>&pose,uint32_t groups,const float*position,float yaw,SC_Mesh*m)const{
  if(!m||!m->position||!m->normal||!m->color||!m->uv||pose.size()!=bones*12)return false;
  size_t count=0;for(const auto&v:vertices)if(v.group&groups)count++;if(count/3>m->capacityTriangles)return false;
  float cs=cosf(yaw),sn=sinf(yaw);size_t out=0;
  for(const auto&v:vertices){if(!(v.group&groups))continue;float p[3]={},n[3]={};
   for(int j=0;j<4;j++)if(v.weight[j]>0){const float*t=pose.data()+v.bone[j]*12;for(int a=0;a<3;a++){p[a]+=(t[a*4]*v.p[0]+t[a*4+1]*v.p[1]+t[a*4+2]*v.p[2]+t[a*4+3])*v.weight[j];n[a]+=(t[a*4]*v.n[0]+t[a*4+1]*v.n[1]+t[a*4+2]*v.n[2])*v.weight[j];}}
   float len=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(len>.001f)for(float&x:n)x/=len;
   m->position[out*3]=position[0]+cs*p[0]+sn*p[2];m->position[out*3+1]=position[1]+p[1];m->position[out*3+2]=position[2]-sn*p[0]+cs*p[2];
   m->normal[out*3]=cs*n[0]+sn*n[2];m->normal[out*3+1]=n[1];m->normal[out*3+2]=-sn*n[0]+cs*n[2];
   memcpy(m->color+out*3,v.color,12);memcpy(m->uv+out*2,v.uv,8);out++;
  }m->numTrianglesUsed=uint32_t(out/3);return true;
 }
};
}
