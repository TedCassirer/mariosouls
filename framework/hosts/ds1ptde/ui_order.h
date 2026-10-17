#include "../../core/host_capabilities.h"
// Verified PTDE layout: final scene copy, SHORT2 UI sprites, then text.
// Install on the real device; its draw functions differ from the probe device.
static bool uiComposited = false, uiInside = false, uiHooksReady = false;

static void uiPresent() {
    classPreviewRendered = false;
    uiComposited = false;
}
enum class UILayout { Other, SceneCopy, Sprite, Text };

static UILayout uiLayout(IDirect3DDevice9 *d, UINT stride) {
    if (stride != 12 && stride != 24)
        return UILayout::Other;
    IDirect3DVertexDeclaration9 *decl = nullptr;
    if (FAILED(d->GetVertexDeclaration(&decl)) || !decl)
        return UILayout::Other;
    D3DVERTEXELEMENT9 e[MAXD3DDECLLENGTH] = {};
    UINT n = MAXD3DDECLLENGTH;
    HRESULT hr = decl->GetDeclaration(e, &n);
    decl->Release();
    if (FAILED(hr) || n != 4 || e[3].Stream != 0xff)
        return UILayout::Other;
    for (int i = 0; i < 3; ++i)
        if (e[i].Stream || e[i].UsageIndex || e[i].Method != D3DDECLMETHOD_DEFAULT)
            return UILayout::Other;
    auto elem = [&](int i, WORD offset, BYTE type, BYTE usage) {
        return e[i].Offset == offset && e[i].Type == type && e[i].Usage == usage;
    };
    if (stride == 12 && elem(0, 0, D3DDECLTYPE_SHORT2, D3DDECLUSAGE_POSITION) &&
        elem(1, 4, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR) &&
        elem(2, 8, D3DDECLTYPE_SHORT2, D3DDECLUSAGE_TEXCOORD))
        return UILayout::Sprite;
    if (stride == 24 && elem(0, 0, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION)) {
        if (elem(1, 12, D3DDECLTYPE_UBYTE4N, D3DDECLUSAGE_COLOR) &&
            elem(2, 16, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD))
            return UILayout::SceneCopy;
        if (elem(1, 12, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD) &&
            elem(2, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR))
            return UILayout::Text;
    }
    return UILayout::Other;
}

static UILayout uiBoundary(IDirect3DDevice9 *d, UINT stride) {
    if (uiInside || uiComposited || renderMode != 2 || !renderWanted() || !sceneDepth)
        return UILayout::Other;
    auto layout = uiLayout(d, stride);
    if (layout == UILayout::Other)
        return layout;
    DWORD z = 0, blend = 0;
    d->GetRenderState(D3DRS_ZENABLE, &z);
    d->GetRenderState(D3DRS_ALPHABLENDENABLE, &blend);
    if (z || (layout == UILayout::SceneCopy ? blend != FALSE : blend != TRUE))
        return UILayout::Other;
    IDirect3DSurface9 *rt = nullptr;
    D3DSURFACE_DESC r = {};
    d->GetRenderTarget(0, &rt);
    if (rt) {
        rt->GetDesc(&r);
        rt->Release();
    }
    D3DVIEWPORT9 vp = {};
    d->GetViewport(&vp);
    // PTDE draws its UI into an intermediate target, then scales it to the
    // window. Its dimensions need not equal the swap-chain backbuffer.
    if (!SCRender::uiTarget(r, vp))
        return UILayout::Other;
    return layout;
}

static void uiComposite(IDirect3DDevice9 *d, UILayout layout) {
    auto capabilities = hostCapabilities(layout != UILayout::Other);
    if (uiComposited || uiInside || layout == UILayout::Other ||
        !SCHost::permits(capabilities, SCHost::Operation::WorldDraw))
        return;
    uiInside = true;
    uiComposited = drawComposite(d);
    uiInside = false;
}

using UIPrimitiveFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, const void *,
                                        UINT);
using UIIndexedFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, UINT, UINT,
                                      const void *, D3DFORMAT, const void *, UINT);
static UIPrimitiveFn uiOriginalPrimitive;
static UIIndexedFn uiOriginalIndexed;

static HRESULT WINAPI uiPrimitive(IDirect3DDevice9 *d, D3DPRIMITIVETYPE type, UINT count,
                                  const void *v, UINT stride) {
    auto layout = uiBoundary(d, stride);
    if (layout == UILayout::Text || layout == UILayout::Sprite)
        uiComposite(d, layout);
    auto nativePreview =
        uiLayout(d, stride) == UILayout::Sprite
            ? classPreviewBind(d, count, v, stride,
                               (type == D3DPT_TRIANGLESTRIP || type == D3DPT_TRIANGLEFAN)
                                   ? count + 2
                                   : count * 3)
            : nullptr;
    HRESULT hr = uiOriginalPrimitive(d, type, count, v, stride);
    if (nativePreview) {
        d->SetTexture(0, nativePreview);
        nativePreview->Release();
    }
    if (SUCCEEDED(hr) && layout == UILayout::SceneCopy &&
        (type == D3DPT_TRIANGLESTRIP || type == D3DPT_TRIANGLELIST) && count == 2)
        uiComposite(d, layout);
    return hr;
}

static HRESULT WINAPI uiIndexed(IDirect3DDevice9 *d, D3DPRIMITIVETYPE type, UINT min, UINT num,
                                UINT count, const void *indices, D3DFORMAT format, const void *v,
                                UINT stride) {
    auto layout = uiBoundary(d, stride);
    if (layout == UILayout::Text || layout == UILayout::Sprite)
        uiComposite(d, layout);
    auto nativePreview = uiLayout(d, stride) == UILayout::Sprite
                             ? classPreviewBind(d, count, v, stride, num)
                             : nullptr;
    HRESULT hr = uiOriginalIndexed(d, type, min, num, count, indices, format, v, stride);
    if (nativePreview) {
        d->SetTexture(0, nativePreview);
        nativePreview->Release();
    }
    if (SUCCEEDED(hr) && layout == UILayout::SceneCopy &&
        (type == D3DPT_TRIANGLESTRIP || type == D3DPT_TRIANGLELIST) && count == 2)
        uiComposite(d, layout);
    return hr;
}

static void installUIHooks(void **table) {
    auto a = MH_CreateHook(table[83], (void *)uiPrimitive, (void **)&uiOriginalPrimitive);
    auto b = MH_CreateHook(table[84], (void *)uiIndexed, (void **)&uiOriginalIndexed);
    if (a == MH_OK && b == MH_OK) {
        uiHooksReady = MH_EnableHook(table[83]) == MH_OK && MH_EnableHook(table[84]) == MH_OK;
    }
    logMessage("UI boundary hooks primitive=%d indexed=%d ready=%d", a, b, uiHooksReady);
}
