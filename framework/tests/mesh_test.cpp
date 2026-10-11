#undef NDEBUG
#include "../core/mesh_pack.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(int argc,char**argv){
 assert(argc==3);SCMesh::Pack p;assert(p.load(argv[1]));
 std::vector<float> positions(SC_MAX_TRIANGLES*9),normal(positions.size()),color(positions.size()),uv(SC_MAX_TRIANGLES*6),pose;
 SC_Mesh mesh={positions.data(),normal.data(),color.data(),uv.data(),SC_MAX_TRIANGLES,0};float origin[3]={};
 unsigned frames=0;
 for(size_t clip=0;clip<p.clips.size();clip++)for(unsigned f=0;f<p.clips[clip].frames;f++){
  assert(p.sample(int(clip),float(f),pose));assert(p.draw(pose,~0u,origin,0,&mesh));
  for(unsigned k=0;k<mesh.numTrianglesUsed*9;k++)assert(std::isfinite(positions[k])&&fabsf(positions[k])<1000);
  frames++;
 }
 assert(!p.sample(-1,0,pose));assert(!p.sample(0,std::numeric_limits<float>::quiet_NaN(),pose));
 FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);std::vector<unsigned char>bytes(size);assert(fread(bytes.data(),1,size,f)==size_t(size));fclose(f);
 auto invalid=[&](const std::vector<unsigned char>&b){FILE*out=fopen(argv[2],"wb");assert(out);fwrite(b.data(),1,b.size(),out);fclose(out);SCMesh::Pack bad;assert(!bad.load(argv[2]));};
 auto corrupt=bytes;corrupt.resize(128);invalid(corrupt);
 corrupt=bytes;memset(corrupt.data()+16,255,4);invalid(corrupt);
 corrupt=bytes;size_t start=28+p.width*p.height*4;float nan=std::numeric_limits<float>::quiet_NaN();memcpy(corrupt.data()+start,&nan,4);invalid(corrupt);
 corrupt=bytes;uint16_t bone=65535;memcpy(corrupt.data()+start+44,&bone,2);invalid(corrupt);
 corrupt=bytes;float weight=-1;memcpy(corrupt.data()+start+52,&weight,4);invalid(corrupt);
 corrupt=bytes;corrupt.push_back(0);invalid(corrupt);remove(argv[2]);
 printf("PASS real mesh: %u animation frames bounded; truncated/overflow/NaN/bone/weight/trailing corruption rejected\n",frames);
}
