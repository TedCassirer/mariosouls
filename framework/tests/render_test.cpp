// Real D3D9, hidden window, synthetic geometry only. Never opens the game.
#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../hosts/ds1ptde/render_surfaces.h"
static bool renderWanted(){return true;}
static bool failDraw=false;
static bool draw(IDirect3DDevice9*d){
 if(failDraw)return false;
 D3DVIEWPORT9 v={};d->GetViewport(&v);
 struct V{float x,y,z,w;DWORD c;};
 V q[]={{-.5f,-.5f,.5f,1,0xffff0000},{v.Width-.5f,-.5f,.5f,1,0xffff0000},{-.5f,v.Height-.5f,.5f,1,0xffff0000},{v.Width-.5f,v.Height-.5f,.5f,1,0xffff0000}};
 d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
 d->SetTexture(0,nullptr);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
 d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
 d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);
 return SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,q,sizeof(V)));
}
#include "../hosts/ds1ptde/render_composite.h"
static bool classPreviewRendered=false;
static void classPreviewReset(){}
static IDirect3DBaseTexture9*classPreviewBind(IDirect3DDevice9*,UINT,const void*,UINT,UINT){return nullptr;}
static int renderMode=2;static const char*ini="";
static void logf(const char*,...){}
static int MH_CreateHook(void*,void*,void**){return 0;}
static int MH_EnableHook(void*){return 0;}
static const int MH_OK=0;
#include "../hosts/ds1ptde/ui_order.h"
static void ok(HRESULT hr){if(FAILED(hr))printf("D3D failure %08lx\n",hr);assert(SUCCEEDED(hr));}
static DWORD pixel(IDirect3DDevice9*d,IDirect3DSurface9*s,UINT x,UINT y){
 D3DSURFACE_DESC desc={};s->GetDesc(&desc);IDirect3DSurface9*cpu=nullptr;
 ok(d->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&cpu,nullptr));ok(d->GetRenderTargetData(s,cpu));
 D3DLOCKED_RECT lock={};ok(cpu->LockRect(&lock,nullptr,D3DLOCK_READONLY));DWORD p=((DWORD*)((BYTE*)lock.pBits+y*lock.Pitch))[x];cpu->UnlockRect();cpu->Release();return p&0xffffff;
}
int main(){
 WNDCLASSA wc={};wc.lpfnWndProc=DefWindowProcA;wc.hInstance=GetModuleHandleA(nullptr);wc.lpszClassName="SCOfflineRenderTest";RegisterClassA(&wc);
 HWND w=CreateWindowA(wc.lpszClassName,"",WS_OVERLAPPED,0,0,32,32,nullptr,nullptr,wc.hInstance,nullptr);assert(w);
 IDirect3D9*api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);IDirect3DDevice9*d=nullptr;
 D3DPRESENT_PARAMETERS pp={};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=w;pp.BackBufferWidth=32;pp.BackBufferHeight=32;
 ok(api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d));
 D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_UBYTE4N,0,D3DDECLUSAGE_COLOR,0},{0,16,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
 IDirect3DVertexDeclaration9*decl=nullptr;ok(d->CreateVertexDeclaration(elements,&decl));
 struct Size{UINT sceneW,sceneH,uiW,uiH;D3DFORMAT format;};
 for(auto size:{Size{1024,720,1280,720,D3DFMT_A8R8G8B8},Size{1024,720,1920,1080,D3DFMT_X8R8G8B8},Size{1920,1080,2560,1440,D3DFMT_A8R8G8B8},Size{2560,1440,3840,2160,D3DFMT_A8R8G8B8},Size{1024,720,800,600,D3DFMT_A8R8G8B8}}){
  IDirect3DSurface9*color=nullptr,*depth=nullptr,*target=nullptr;
  ok(d->CreateRenderTarget(size.sceneW,size.sceneH,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&color,nullptr));
  ok(d->CreateDepthStencilSurface(size.sceneW,size.sceneH,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr));
  ok(d->CreateRenderTarget(size.uiW,size.uiH,size.format,D3DMULTISAMPLE_NONE,0,FALSE,&target,nullptr));
  ok(d->SetDepthStencilSurface(nullptr));ok(d->SetRenderTarget(0,color));ok(d->SetDepthStencilSurface(depth));
  ok(d->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1,0));D3DRECT left={0,0,LONG(size.sceneW/2),LONG(size.sceneH)};ok(d->Clear(1,&left,D3DCLEAR_ZBUFFER,0,.2f,0));
  renderDepthObserve(d);assert(sceneDepth==depth);
  IDirect3DSurface9*smallColor=nullptr,*smallDepth=nullptr;
  ok(d->CreateRenderTarget(256,180,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&smallColor,nullptr));
  ok(d->CreateDepthStencilSurface(256,180,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&smallDepth,nullptr));
  ok(d->SetDepthStencilSurface(nullptr));ok(d->SetRenderTarget(0,smallColor));ok(d->SetDepthStencilSurface(smallDepth));
  renderDepthObserve(d);assert(sceneDepth==depth);smallColor->Release();smallDepth->Release();
  // The observed shadow and smaller-effect sizes must not replace scene depth.
  D3DSURFACE_DESC shadow={};shadow.Width=shadow.Height=2048;shadow.Format=D3DFMT_A8R8G8B8;D3DVIEWPORT9 sv={0,0,2048,2048,0,1};assert(!SCRender::scene(shadow,shadow,sv));
  ok(d->SetDepthStencilSurface(nullptr));ok(d->SetRenderTarget(0,target));ok(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff0000ff,1,0));
  ok(d->SetVertexDeclaration(decl));ok(d->SetRenderState(D3DRS_ZENABLE,FALSE));ok(d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE));
  uiPresent();assert(uiBoundary(d,24)==UILayout::SceneCopy); // UI target intentionally differs from backbuffer.
  ok(d->BeginScene());failDraw=true;uiComposite(d,UILayout::SceneCopy);assert(!uiComposited);failDraw=false;
  uiComposite(d,UILayout::SceneCopy);assert(uiComposited);ok(d->EndScene());
  assert(pixel(d,target,size.uiW/4,size.uiH/2)==0x0000ff);assert(pixel(d,target,size.uiW*3/4,size.uiH/2)==0xff0000);
  IDirect3DSurface9*restored=nullptr;d->GetRenderTarget(0,&restored);assert(restored==target);restored->Release();
  D3DVIEWPORT9 viewport={};d->GetViewport(&viewport);assert(viewport.Width==size.uiW&&viewport.Height==size.uiH);
  D3DRECT text={0,LONG(size.uiH/2),LONG(size.uiW),LONG(size.uiH)};ok(d->Clear(1,&text,D3DCLEAR_TARGET,0xff00ff00,1,0));
  assert(pixel(d,target,size.uiW*3/4,size.uiH*3/4)==0x00ff00);assert(uiBoundary(d,24)==UILayout::Other);
  printf("PASS composition %ux%u -> %ux%u format=%u, occlusion, UI priority, failed-draw retry\n",size.sceneW,size.sceneH,size.uiW,size.uiH,unsigned(size.format));
  renderDepthEndFrame();assert(!sceneDepth&&!sceneDepthArea);color->Release();depth->Release();target->Release();
 }
 renderSurfacesReset();assert(!characterLayer);
 IDirect3DSurface9*back=nullptr;ok(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back));ok(d->SetRenderTarget(0,back));back->Release();
 pp.BackBufferWidth=64;pp.BackBufferHeight=64;ok(d->Reset(&pp));
 decl->Release();d->Release();api->Release();DestroyWindow(w);
 puts("Offline D3D9 render checks passed; live PTDE remains a separate check.");
}
