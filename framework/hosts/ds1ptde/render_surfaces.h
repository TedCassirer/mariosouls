#pragma once

namespace SCRender {
inline bool color(D3DFORMAT f) {
    return f == D3DFMT_A8R8G8B8 || f == D3DFMT_X8R8G8B8;
}

inline bool full(const D3DSURFACE_DESC &s, const D3DVIEWPORT9 &v) {
    return s.Width && s.Height && !v.X && !v.Y && v.Width == s.Width && v.Height == s.Height;
}

inline bool uiTarget(const D3DSURFACE_DESC &s, const D3DVIEWPORT9 &v) {
    return color(s.Format) && full(s, v);
}

inline bool scene(const D3DSURFACE_DESC &s, const D3DSURFACE_DESC &z, const D3DVIEWPORT9 &v) {
    // Stock PTDE: square shadow/portrait targets, rectangular scene and
    // downsampled effects. Keep the largest rectangular depth pass this frame;
    // later equal-sized passes refresh it without selecting smaller effects.
    return uiTarget(s, v) && s.Width > s.Height && s.Width == z.Width && s.Height == z.Height &&
           s.MultiSampleType == D3DMULTISAMPLE_NONE && z.MultiSampleType == D3DMULTISAMPLE_NONE;
}
} // namespace SCRender

static IDirect3DSurface9 *sceneDepth = nullptr;
static IDirect3DTexture9 *characterLayer = nullptr;
static uint64_t sceneDepthArea = 0;
static int diagnosticSceneWidth = 0, diagnosticSceneHeight = 0;

static void renderDepthEndFrame() {
    if (sceneDepth) {
        sceneDepth->Release();
        sceneDepth = nullptr;
    }
    sceneDepthArea = 0;
}

static void renderSurfacesReset() {
    diagnosticSceneWidth = diagnosticSceneHeight = 0;
    renderDepthEndFrame();
    if (characterLayer) {
        characterLayer->Release();
        characterLayer = nullptr;
    }
}

static void renderDepthObserve(IDirect3DDevice9 *d) {
    IDirect3DSurface9 *color = nullptr, *depth = nullptr;
    D3DSURFACE_DESC c = {}, z = {};
    D3DVIEWPORT9 v = {};
    d->GetRenderTarget(0, &color);
    d->GetDepthStencilSurface(&depth);
    d->GetViewport(&v);
    if (color)
        color->GetDesc(&c);
    if (depth)
        depth->GetDesc(&z);
    uint64_t area = uint64_t(c.Width) * c.Height;
    if (depth && SCRender::scene(c, z, v) && area >= sceneDepthArea) {
        diagnosticSceneWidth = int(c.Width);
        diagnosticSceneHeight = int(c.Height);
        if (sceneDepth)
            sceneDepth->Release();
        sceneDepth = depth;
        depth = nullptr;
        sceneDepthArea = area;
    }
    if (color)
        color->Release();
    if (depth)
        depth->Release();
}
