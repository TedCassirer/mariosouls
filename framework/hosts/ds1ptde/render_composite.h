// Composite at the native UI boundary, using the current scene depth size.
static bool drawComposite(IDirect3DDevice9 *d) {
    if (!renderWanted() || !sceneDepth)
        return false;
    D3DSURFACE_DESC scene = {};
    if (FAILED(sceneDepth->GetDesc(&scene)) || !scene.Width || !scene.Height)
        return false;
    D3DSURFACE_DESC existing = {};
    if (characterLayer)
        characterLayer->GetLevelDesc(0, &existing);
    if (characterLayer && (existing.Width != scene.Width || existing.Height != scene.Height)) {
        characterLayer->Release();
        characterLayer = nullptr;
    }
    if (!characterLayer &&
        FAILED(d->CreateTexture(scene.Width, scene.Height, 1, D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &characterLayer, nullptr)))
        return false;
    IDirect3DSurface9 *target = nullptr, *depth = nullptr, *layer = nullptr;
    IDirect3DStateBlock9 *saved = nullptr;
    d->GetRenderTarget(0, &target);
    d->GetDepthStencilSurface(&depth);
    characterLayer->GetSurfaceLevel(0, &layer);
    if (!target || !layer || FAILED(d->CreateStateBlock(D3DSBT_ALL, &saved))) {
        if (target)
            target->Release();
        if (depth)
            depth->Release();
        if (layer)
            layer->Release();
        return false;
    }
    IDirect3DVertexDeclaration9 *uiDeclaration = nullptr;
    d->GetVertexDeclaration(&uiDeclaration);
    D3DMATRIX uiWorld, uiView, uiProjection;
    d->GetTransform(D3DTS_WORLD, &uiWorld);
    d->GetTransform(D3DTS_VIEW, &uiView);
    d->GetTransform(D3DTS_PROJECTION, &uiProjection);
    saved->Capture();
    D3DVIEWPORT9 output;
    d->GetViewport(&output);
    D3DVIEWPORT9 vp = {0, 0, scene.Width, scene.Height, 0, 1};
    bool rendered =
        SUCCEEDED(d->SetDepthStencilSurface(nullptr)) && SUCCEEDED(d->SetRenderTarget(0, layer)) &&
        SUCCEEDED(d->SetDepthStencilSurface(sceneDepth)) && SUCCEEDED(d->SetViewport(&vp)) &&
        SUCCEEDED(d->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1, 0));
    if (rendered)
        rendered = draw(d);
    d->SetDepthStencilSurface(nullptr);
    d->SetRenderTarget(0, target);
    d->SetDepthStencilSurface(depth);
    d->SetViewport(&output);

    struct Quad {
        float x, y, z, w, u, v;
    };

    float x = float(output.X) - .5f, y = float(output.Y) - .5f, w = float(output.Width),
          h = float(output.Height);
    Quad q[] = {{x, y, 0, 1, 0, 0},
                {x + w, y, 0, 1, 1, 0},
                {x, y + h, 0, 1, 0, 1},
                {x + w, y + h, 0, 1, 1, 1}};
    d->SetVertexShader(nullptr);
    d->SetPixelShader(nullptr);
    d->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    d->SetTexture(0, characterLayer);
    d->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    d->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    d->SetRenderState(D3DRS_ZENABLE, FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d->SetRenderState(D3DRS_LIGHTING, FALSE);
    d->SetRenderState(D3DRS_FOGENABLE, FALSE);
    d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    d->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    d->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    if (rendered)
        rendered = SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(Quad)));
    saved->Apply();
    d->SetTransform(D3DTS_WORLD, &uiWorld);
    d->SetTransform(D3DTS_VIEW, &uiView);
    d->SetTransform(D3DTS_PROJECTION, &uiProjection);
    if (uiDeclaration) {
        d->SetVertexDeclaration(uiDeclaration);
        uiDeclaration->Release();
    }
    saved->Release();
    target->Release();
    if (depth)
        depth->Release();
    layer->Release();
    return rendered;
}
