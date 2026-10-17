#pragma once
#include "../../sdk/render_api.h"

// D3D9 resources and fixed-function state stay behind the presentation boundary.
// CPU images are immutable and remain alive for the host session.
namespace PTDEGraphics {
struct Image {
    IDirect3DDevice9 *device;
    const uint8_t *pixels;
    uint32_t width, height;
    IDirect3DTexture9 *texture;
};

static std::vector<Image> images;

static void resetTextures() {
    for (auto &image : images)
        image.texture->Release();
    images.clear();
}

static IDirect3DTexture9 *texture(IDirect3DDevice9 *device, SCRenderAPI::Texture t) {
    if (!t.rgba || !t.width || !t.height || t.width > 4096 || t.height > 4096)
        return nullptr;
    if (!images.empty() && images.front().device != device)
        resetTextures();
    for (const auto &image : images)
        if (image.pixels == t.rgba && image.width == t.width && image.height == t.height)
            return image.texture;
    IDirect3DTexture9 *result = nullptr;
    if (FAILED(device->CreateTexture(t.width, t.height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                     &result, nullptr)))
        return nullptr;
    D3DLOCKED_RECT lr;
    if (FAILED(result->LockRect(0, &lr, nullptr, 0))) {
        result->Release();
        return nullptr;
    }
    for (uint32_t y = 0; y < t.height; ++y) {
        auto row = reinterpret_cast<uint32_t *>(static_cast<BYTE *>(lr.pBits) + y * lr.Pitch);
        for (uint32_t x = 0; x < t.width; ++x) {
            auto p = &t.rgba[(y * t.width + x) * 4];
            row[x] = (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
        }
    }
    result->UnlockRect(0);
    images.push_back({device, t.rgba, t.width, t.height, result});
    return result;
}
} // namespace PTDEGraphics

class PTDERenderer final : public SCRenderAPI::Renderer {
    IDirect3DDevice9 *device;

    static bool allowed(SCRenderAPI::Context c) {
        return c.phase != SCRenderAPI::Phase::BeforeHUD || c.depthAvailable;
    }

    bool material(const SCRenderAPI::Material &m, bool screen = false) {
        using namespace SCRenderAPI;
        IDirect3DTexture9 *image = nullptr;
        if (m.color != ColorMode::Vertex) {
            image = PTDEGraphics::texture(device, m.image);
            if (!image)
                return false;
        }
        device->SetVertexShader(nullptr);
        device->SetPixelShader(nullptr);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_ZENABLE, !screen && m.depthTest);
        device->SetRenderState(D3DRS_ZWRITEENABLE, !screen && m.depthWrite);
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, m.alphaBlend);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, m.alphaCutoff != 0);
        device->SetRenderState(D3DRS_ALPHAREF, m.alphaCutoff);
        device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
        device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
        device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
        device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTexture(0, image);
        device->SetTextureStageState(0, D3DTSS_COLOROP,
                                     m.color == ColorMode::Vertex     ? D3DTOP_SELECTARG1
                                     : m.color == ColorMode::Modulate ? D3DTOP_MODULATE
                                                                      : D3DTOP_BLENDTEXTUREALPHA);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, image ? D3DTA_TEXTURE : D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1,
                                     image && m.alphaCutoff ? D3DTA_TEXTURE : D3DTA_DIFFUSE);
        device->SetTextureStageState(1, D3DTSS_COLOROP, m.shade ? D3DTOP_MODULATE : D3DTOP_DISABLE);
        device->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
        device->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_SPECULAR);
        device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
        device->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
        auto filter = m.filter == Filter::Point ? D3DTEXF_POINT : D3DTEXF_LINEAR;
        device->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        if (m.alphaBlend) {
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
            device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
            device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
            device->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
        }
        return true;
    }

public:
    explicit PTDERenderer(IDirect3DDevice9 *d) : device(d) {}

    bool texture(SCRenderAPI::Texture t) override {
        return PTDEGraphics::texture(device, t) != nullptr;
    }

    SCRenderAPI::Viewport viewport() const override {
        D3DVIEWPORT9 vp{};
        device->GetViewport(&vp);
        return {float(vp.X), float(vp.Y), float(vp.Width), float(vp.Height)};
    }

    void camera(const SCRenderAPI::Camera &c) override {
        static_assert(sizeof(D3DMATRIX) == sizeof(c.world), "Matrix layout");
        D3DMATRIX world, view, projection;
        memcpy(&world, c.world, sizeof(world));
        memcpy(&view, c.view, sizeof(view));
        memcpy(&projection, c.projection, sizeof(projection));
        device->SetTransform(D3DTS_WORLD, &world);
        device->SetTransform(D3DTS_VIEW, &view);
        device->SetTransform(D3DTS_PROJECTION, &projection);
    }

    bool mesh(SCRenderAPI::Mesh m, const SCRenderAPI::Material &style,
              SCRenderAPI::Context c) override {
        if (!allowed(c) || !material(style))
            return false;
        device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
        return !m.triangles ||
               (m.vertices &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m.triangles, m.vertices,
                                                  sizeof(SCRenderAPI::Vertex))));
    }

    bool lines(SCRenderAPI::Lines m, bool depthTest, SCRenderAPI::Context c) override {
        SCRenderAPI::Material style{};
        style.depthTest = depthTest;
        style.depthWrite = false;
        if (!allowed(c) || !material(style))
            return false;
        device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        return !m.segments ||
               (m.vertices &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_LINELIST, m.segments, m.vertices,
                                                  sizeof(SCRenderAPI::LineVertex))));
    }

    bool screen(SCRenderAPI::ScreenMesh m, const SCRenderAPI::Material &style,
                SCRenderAPI::Context c) override {
        if (!allowed(c) || !material(style, true))
            return false;
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        return !m.triangles ||
               (m.vertices &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m.triangles, m.vertices,
                                                  sizeof(SCRenderAPI::ScreenVertex))));
    }
};
