#pragma once
// Substitute the native full-body panel texture. The native UI owns placement,
// clipping and composition; this is a live 3D mesh, not a screen overlay.
static IDirect3DTexture9* classPreviewTexture=nullptr;
static IDirect3DSurface9* classPreviewDepth=nullptr;
static bool classPreviewRendered=false,classPreviewDrawing=false;
static void classPreviewReset(){
 if(classPreviewTexture){classPreviewTexture->Release();classPreviewTexture=nullptr;}
 if(classPreviewDepth){classPreviewDepth->Release();classPreviewDepth=nullptr;}
 classPreviewRendered=false;
}
static bool classPreviewRender(IDirect3DDevice9*d){
 if(classPreviewRendered)return true;
 if(!classPreviewActive||classPreviewDrawing||!geo.numTrianglesUsed)return false;
 if(!classPreviewTexture&&FAILED(d->CreateTexture(512,512,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&classPreviewTexture,nullptr)))return false;
 if(!classPreviewDepth&&FAILED(d->CreateDepthStencilSurface(512,512,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&classPreviewDepth,nullptr)))return false;
 IDirect3DSurface9*target=nullptr,*depth=nullptr,*layer=nullptr;IDirect3DStateBlock9*saved=nullptr;
 d->GetRenderTarget(0,&target);d->GetDepthStencilSurface(&depth);classPreviewTexture->GetSurfaceLevel(0,&layer);
 if(!target||!layer||FAILED(d->CreateStateBlock(D3DSBT_ALL,&saved))){if(target)target->Release();if(depth)depth->Release();if(layer)layer->Release();return false;}
 D3DMATRIX world,view,projection;d->GetTransform(D3DTS_WORLD,&world);d->GetTransform(D3DTS_VIEW,&view);d->GetTransform(D3DTS_PROJECTION,&projection);
 D3DVIEWPORT9 original;d->GetViewport(&original);IDirect3DVertexDeclaration9*decl=nullptr;d->GetVertexDeclaration(&decl);
 saved->Capture();classPreviewDrawing=true;
 d->SetTexture(0,nullptr);d->SetDepthStencilSurface(nullptr);
 bool ok=SUCCEEDED(d->SetRenderTarget(0,layer))&&SUCCEEDED(d->SetDepthStencilSurface(classPreviewDepth));
 if(ok){D3DVIEWPORT9 vp={0,0,512,512,0,1};d->SetViewport(&vp);d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0);draw(d,true);}
 d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,target);d->SetDepthStencilSurface(depth);
 saved->Apply();d->SetViewport(&original);d->SetTransform(D3DTS_WORLD,&world);d->SetTransform(D3DTS_VIEW,&view);d->SetTransform(D3DTS_PROJECTION,&projection);
 if(decl){d->SetVertexDeclaration(decl);decl->Release();}saved->Release();target->Release();if(depth)depth->Release();layer->Release();
 classPreviewDrawing=false;classPreviewRendered=ok;return ok;
}
static IDirect3DBaseTexture9* classPreviewBind(IDirect3DDevice9*d,UINT count,const void*v,UINT stride,UINT vertices){
 if(!classPreviewActive||classPreviewDrawing||!classCreationMenu()||!classSelected()||stride!=12||count!=2||vertices!=4||!v)return nullptr;
 // Verified SHORT2 panel quad, in eighths of a logical UI pixel. Do not replace
 // icons or unrelated textures; creation portraits may use different insets.
 int left=32767,top=32767,right=-32768,bottom=-32768;
 for(int i=0;i<4;i++){auto p=(const short*)((const char*)v+i*stride);left=std::min(left,int(p[0]));right=std::max(right,int(p[0]));top=std::min(top,int(p[1]));bottom=std::max(bottom,int(p[1]));}
 IDirect3DBaseTexture9* native=nullptr;d->GetTexture(0,&native);if(!native)return nullptr;
 D3DSURFACE_DESC desc={};if(native->GetType()==D3DRTYPE_TEXTURE)((IDirect3DTexture9*)native)->GetLevelDesc(0,&desc);
 if(desc.Width!=512||desc.Height!=512||desc.Format!=D3DFMT_A8R8G8B8||!(desc.Usage&D3DUSAGE_RENDERTARGET)){native->Release();return nullptr;}
 static std::vector<std::vector<int>> seen;
 std::vector<int> key={left,top,right,bottom};
 if(seen.size()<16&&std::find(seen.begin(),seen.end(),key)==seen.end()){seen.push_back(key);logf("CLASS portrait bounds=%d,%d,%d,%d",left,top,right,bottom);}
 // Native clothed, physique and face views use different large quads inside
 // the right portrait pane. Texture type/usage and pane bounds exclude icons.
 if(left<6400||right>9800||top<1000||bottom>5200||right-left<2000||bottom-top<2500){native->Release();return nullptr;}
 classPreviewAspect=float(right-left)/float(bottom-top);classPreviewFace=top>=1600;
 if(!classPreviewRender(d)){native->Release();return nullptr;}
 d->SetTexture(0,classPreviewTexture);
 static bool logged=false;if(!logged){logf("CLASS animated preview substituted in native model panel");logged=true;}
 return native;
}
