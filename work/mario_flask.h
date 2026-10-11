#pragma once
// An original low-poly flask in the right-hand joint's local coordinates.
// The native item action alone consumes charges and heals.
static void actionFlask(){
 float m[16];sm_hand(m);Vec axis=unit({.09665986f,-.21849427f,-.9710392f});
 Vec u=unit(cross(axis,{1,0,0})),v=cross(axis,u),grip={40,0,0};
 auto transform=[&](Vec p){return Vec{p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};};
 const float heights[]={-125,-110,-50,-15,0,70,75};
 const float radii[]={0,45,42,30,15,15,0};
 for(int band=0;band<6;band++)for(int side=0;side<10;side++){
  float a=side*6.2831853f/10,b=(side+1)*6.2831853f/10;
  Vec ringA=u*cosf(a)+v*sinf(a),ringB=u*cosf(b)+v*sinf(b);
  Vec q[4]={transform(grip+axis*heights[band]+ringA*radii[band]),transform(grip+axis*heights[band]+ringB*radii[band]),transform(grip+axis*heights[band+1]+ringB*radii[band+1]),transform(grip+axis*heights[band+1]+ringA*radii[band+1])};
  for(int half=0;half<2;half++){
   if(geo.numTrianglesUsed>=SM64_GEO_MAX_TRIANGLES)return;
   int ids[3]={0,half?2:1,half?3:2};Vec n=unit(cross(q[ids[1]]-q[0],q[ids[2]]-q[0]));
   for(int k=0;k<3;k++){size_t i=geo.numTrianglesUsed*3+k;Vec p=q[ids[k]];gpos[i*3]=p.x;gpos[i*3+1]=p.y;gpos[i*3+2]=p.z;gnormal[i*3]=n.x;gnormal[i*3+1]=n.y;gnormal[i*3+2]=n.z;gcolor[i*3]=1.f;gcolor[i*3+1]=band>=4?.75f:.40f;gcolor[i*3+2]=.04f;guv[i*2]=guv[i*2+1]=1.f;}
   ++geo.numTrianglesUsed;
  }
 }
}
