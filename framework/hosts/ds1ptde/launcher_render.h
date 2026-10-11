#pragma once
static bool launcherVisible(){return launcherAvailable&&launcherApi&&characterOwned()&&!nativeActionOwned()&&!characterSession.death();}
static void launcherDrawPart(IDirect3DDevice9*d,unsigned part,Vec at,float yaw,float pitch){
 float viewYaw=atan2f(cameraEye().x-at.x,cameraEye().z-at.z);
 if(!launcherApi->mesh(part,float(GetTickCount()/33),viewYaw,&launcherMesh)||launcherMesh.numTrianglesUsed>launcherMesh.capacityTriangles)return;
 struct V{Vec p;DWORD color;float u,v;};std::vector<V> vertices;vertices.reserve(launcherMesh.numTrianglesUsed*3);
 float cy=cosf(yaw),sy=sinf(yaw),cp=cosf(pitch),sp=sinf(pitch);
 for(unsigned i=0;i<launcherMesh.numTrianglesUsed*3;i++){
  Vec p={launcherPositions[i*3],launcherPositions[i*3+1],launcherPositions[i*3+2]};
  Vec q={p.x,cp*p.y+sp*p.z,-sp*p.y+cp*p.z};p={cy*q.x+sy*q.z,q.y,-sy*q.x+cy*q.z};
  if(!isfinite(dot(p,p))||dot(p,p)>10000000)return;
  int r=std::clamp(int(launcherColors[i*3]*255),0,255),g=std::clamp(int(launcherColors[i*3+1]*255),0,255),b=std::clamp(int(launcherColors[i*3+2]*255),0,255);
  vertices.push_back({at+p*.01f,DWORD(0xff000000|(r<<16)|(g<<8)|b),launcherUV[i*2],launcherUV[i*2+1]});
 }
 d->SetTexture(0,launcherTexture);d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
 d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHAREF,127);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);
 d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,vertices.size()/3,vertices.data(),sizeof(V));
}
static void launcherPrompt(IDirect3DDevice9*d,const char*message){
 if(!message||!message[0]||!debugFontCreate(d))return;
 struct V{float x,y,z,w;DWORD color;float u,v;};std::vector<V> v;
 auto quad=[&](float x,float y,float w,float h,DWORD color,float u,float t,float du,float dt){V a={x,y,0,1,color,u,t},b={x+w,y,0,1,color,u+du,t},c={x,y+h,0,1,color,u,t+dt},e={x+w,y+h,0,1,color,u+du,t+dt};v.insert(v.end(),{a,b,c,b,e,c});};
 float w=float(strlen(message))*9.f+32,x=(1024-w)/2;
 d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetTexture(0,nullptr);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
 quad(x,585,w,38,0xff17191d,0,0,0,0);d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v.data(),sizeof(V));v.clear();
 for(const char*s=message;*s;s++,x+=9){unsigned c=(unsigned char)*s;if(c<32||c>126)c='?';quad(x+16,594,20,20,0xfffff5df,(c%16)/16.f,(c/16)/8.f,1/16.f,1/8.f);}
 d->SetTexture(0,debugFont);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,v.size()/3,v.data(),sizeof(V));
}
static void launcherReticle(IDirect3DDevice9*d){
 if(launcherState.phase!=SC_LAUNCHER_AIMING)return;
 struct V{float x,y,z,w;DWORD color;};std::vector<V> v;
 for(int i=0;i<64;i++){
  float a=i*6.2831853f/64,b=(i+1)*6.2831853f/64;
  V p={512+cosf(a)*240,360+sinf(a)*300,0,1,0xff000000},q={512+cosf(b)*240,360+sinf(b)*300,0,1,0xff000000};
  V r={512+cosf(a)*1600,360+sinf(a)*1600,0,1,0xff000000},s={512+cosf(b)*1600,360+sinf(b)*1600,0,1,0xff000000};v.insert(v.end(),{p,r,q,q,r,s});
 }
 for(int i=0;i<4;i++){
  float a=i*1.5707963f,c=cosf(a),s=sinf(a);
  for(auto p:{Vec{23,0,0},Vec{43,-13,0},Vec{43,13,0}})v.push_back({512+(p.x*c-p.y*s)*.8f,360+p.x*s+p.y*c,0,1,0xb4323232});
 }
 d->SetTexture(0,nullptr);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,v.size()/3,v.data(),sizeof(V));
}
static void launcherDraw(IDirect3DDevice9*d){
 if(!launcherVisible())return;
 if(!launcherTexture){
  if(FAILED(d->CreateTexture(launcherApi->textureWidth,launcherApi->textureHeight,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&launcherTexture,nullptr)))return;
  D3DLOCKED_RECT r;if(FAILED(launcherTexture->LockRect(0,&r,nullptr,0))){launcherTexture->Release();launcherTexture=nullptr;return;}
  for(unsigned y=0;y<launcherApi->textureHeight;y++)for(unsigned x=0;x<launcherApi->textureWidth;x++){auto p=&launcherPixels[(y*launcherApi->textureWidth+x)*4];((DWORD*)((BYTE*)r.pBits+y*r.Pitch))[x]=(p[3]<<24)|(p[0]<<16)|(p[1]<<8)|p[2];}launcherTexture->UnlockRect(0);
 }
 Vec playerAt=read<Vec>(posData+0x10);if(dot(playerAt-launcherPlace,playerAt-launcherPlace)>10000)return;
 launcherDrawPart(d,SC_LAUNCHER_OPERATOR,launcherBuddy,0,0);
 if(!launcherUnlocked)launcherDrawPart(d,SC_LAUNCHER_LID,launcherPlace+Vec{0,.03f,0},0,0);
 else if(launcherState.phase!=SC_LAUNCHER_AIMING){
  Vec at=launcherPlace+Vec{0,-3.35f,0};
  if(launcherState.phase==SC_LAUNCHER_OPENING)at.y=origin.y+launcherState.propLift*.01f;
  float yaw=launcherState.phase==SC_LAUNCHER_OPENING?launcherState.yaw:launcherYaw;
  float pitch=launcherState.phase==SC_LAUNCHER_OPENING?-launcherState.pitch:0;
  launcherDrawPart(d,SC_LAUNCHER_BASE,at,yaw,0);launcherDrawPart(d,SC_LAUNCHER_BARREL,at,yaw,pitch);
 }
 char prompt[160]={};const char*message=nullptr;
 if(launcherState.phase==SC_LAUNCHER_OPENING)message="Preparing cannon...";
 else if(launcherState.phase==SC_LAUNCHER_AIMING)message="Stick / WASD: aim    A / Space: fire    Y / Q: leave";
 else if(launcherMessageUntil&&int32_t(launcherMessageUntil-GetTickCount())>0)message="Ready for blastoff! The cannon is open.";
 else if(launcherTarget==1){snprintf(prompt,sizeof(prompt),"A / Q: Talk to %s",launcherApi->operatorName);message=prompt;}
 else if(launcherTarget==2)message="A / Q: Enter cannon";
 launcherReticle(d);launcherPrompt(d,message);
}
