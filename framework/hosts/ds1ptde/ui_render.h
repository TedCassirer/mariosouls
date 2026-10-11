// PTDE UI draw-boundary tracing. All inspection is bounded to an explicit frame.
static unsigned uiTraceSerial = 0, uiTraceFrames = 0, uiDrawIndex = 0;

static void uiFrameEnd() {
    unsigned serial = GetPrivateProfileIntA("MarioSouls", "UITrace", 0, ini);
    if (serial && serial != uiTraceSerial) {
        uiTraceSerial = serial;
        uiTraceFrames = 2;
    }
}

static void uiPresent() {
    uiDrawIndex = 0;
    if (uiTraceFrames)
        --uiTraceFrames;
}

static void uiInspect(IDirect3DDevice9 *d, const char *kind, UINT count, UINT stride,
                      const void *vertices) {
    if (uiTraceFrames != 1 || uiDrawIndex >= 240)
        return;
    IDirect3DSurface9 *check = nullptr;
    D3DSURFACE_DESC cd = {};
    d->GetRenderTarget(0, &check);
    if (check) {
        check->GetDesc(&cd);
        check->Release();
    }
    if (cd.Width < 1280 || cd.Height < 720)
        return;
    IDirect3DSurface9 *rt = nullptr;
    IDirect3DVertexShader9 *vs = nullptr;
    IDirect3DPixelShader9 *ps = nullptr;
    IDirect3DVertexDeclaration9 *vd = nullptr;
    IDirect3DBaseTexture9 *tex = nullptr;
    D3DSURFACE_DESC rd = {};
    DWORD fvf = 0, z = 0, blend = 0;
    d->GetRenderTarget(0, &rt);
    if (rt)
        rt->GetDesc(&rd);
    d->GetFVF(&fvf);
    d->GetRenderState(D3DRS_ZENABLE, &z);
    d->GetRenderState(D3DRS_ALPHABLENDENABLE, &blend);
    d->GetVertexShader(&vs);
    d->GetPixelShader(&ps);
    d->GetVertexDeclaration(&vd);
    d->GetTexture(0, &tex);
    logf(
        "ui draw=%u kind=%s count=%u stride=%u rt=%ux%u format=%u fvf=%x z=%u blend=%u vs=%p ps=%p tex=%p",
        ++uiDrawIndex, kind, count, stride, rd.Width, rd.Height, rd.Format, fvf, z, blend, vs, ps,
        tex);
    if (vd) {
        D3DVERTEXELEMENT9 es[MAXD3DDECLLENGTH];
        UINT n = MAXD3DDECLLENGTH;
        if (SUCCEEDED(vd->GetDeclaration(es, &n)))
            for (UINT i = 0; i < n; ++i)
                logf("ui element stream=%u offset=%u type=%u usage=%u index=%u", es[i].Stream,
                     es[i].Offset, es[i].Type, es[i].Usage, es[i].UsageIndex);
        vd->Release();
    }
    if (rt)
        rt->Release();
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    if (tex)
        tex->Release();
}

using UIPrimitiveFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, const void *,
                                        UINT);
using UIIndexedFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, UINT, UINT,
                                      const void *, D3DFORMAT, const void *, UINT);
static UIPrimitiveFn uiOriginalPrimitive;
static UIIndexedFn uiOriginalIndexed;
using UIBufferPrimitiveFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, UINT);
using UIBufferIndexedFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, INT, UINT, UINT,
                                            UINT, UINT);
static UIBufferPrimitiveFn uiOriginalBufferPrimitive;
static UIBufferIndexedFn uiOriginalBufferIndexed;

static void uiInspectBuffer(IDirect3DDevice9 *d, const char *kind, UINT count) {
    if (uiTraceFrames != 1)
        return;
    IDirect3DVertexBuffer9 *v = nullptr;
    UINT offset = 0, stride = 0;
    d->GetStreamSource(0, &v, &offset, &stride);
    uiInspect(d, kind, count, stride, nullptr);
    if (v)
        v->Release();
}

static HRESULT WINAPI uiBufferPrimitive(IDirect3DDevice9 *d, D3DPRIMITIVETYPE type, UINT start,
                                        UINT count) {
    uiInspectBuffer(d, "DP", count);
    return uiOriginalBufferPrimitive(d, type, start, count);
}

static HRESULT WINAPI uiBufferIndexed(IDirect3DDevice9 *d, D3DPRIMITIVETYPE type, INT base,
                                      UINT min, UINT num, UINT start, UINT count) {
    uiInspectBuffer(d, "DIP", count);
    return uiOriginalBufferIndexed(d, type, base, min, num, start, count);
}

static HRESULT WINAPI uiPrimitive(IDirect3DDevice9 *d, D3DPRIMITIVETYPE type, UINT count,
                                  const void *v, UINT stride) {
    uiInspect(d, "UP", count, stride, v);
    return uiOriginalPrimitive(d, type, count, v, stride);
}

static HRESULT WINAPI uiIndexed(IDirect3DDevice9 *d, D3DPRIMITIVETYPE type, UINT min, UINT num,
                                UINT count, const void *indices, D3DFORMAT format, const void *v,
                                UINT stride) {
    uiInspect(d, "IUP", count, stride, v);
    return uiOriginalIndexed(d, type, min, num, count, indices, format, v, stride);
}

static void installUIHooks(void **table) {
    auto c =
        MH_CreateHook(table[81], (void *)uiBufferPrimitive, (void **)&uiOriginalBufferPrimitive);
    if (c == MH_OK)
        MH_EnableHook(table[81]);
    auto e = MH_CreateHook(table[82], (void *)uiBufferIndexed, (void **)&uiOriginalBufferIndexed);
    if (e == MH_OK)
        MH_EnableHook(table[82]);
    auto a = MH_CreateHook(table[83], (void *)uiPrimitive, (void **)&uiOriginalPrimitive);
    if (a == MH_OK)
        MH_EnableHook(table[83]);
    auto b = MH_CreateHook(table[84], (void *)uiIndexed, (void **)&uiOriginalIndexed);
    if (b == MH_OK)
        MH_EnableHook(table[84]);
    logf("UI boundary hooks device table=%p DP=%p/%d DIP=%p/%d UP=%p/%d IUP=%p/%d", table,
         table[81], c, table[82], e, table[83], a, table[84], b);
}
