#pragma once
#include "../../sdk/render_api.h"

// Device/resource lifetime, pipeline state, depth capture and HUD detection stay
// with PTDE. This backend borrows the device and the host-owned managed atlas.
class PTDERenderer final : public SCRenderAPI::Renderer {
    IDirect3DDevice9 *device;
    IDirect3DTexture9 *&atlas;

public:
    PTDERenderer(IDirect3DDevice9 *d, IDirect3DTexture9 *&t) : device(d), atlas(t) {}

    bool texture(SCRenderAPI::Texture t) override {
        if (atlas)
            return true;
        if (!t.rgba || !t.width || !t.height)
            return false;
        if (FAILED(device->CreateTexture(t.width, t.height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                         &atlas, nullptr)))
            return false;
        D3DLOCKED_RECT lr;
        if (SUCCEEDED(atlas->LockRect(0, &lr, nullptr, 0))) {
            for (uint32_t y = 0; y < t.height; ++y) {
                auto row =
                    reinterpret_cast<uint32_t *>(static_cast<BYTE *>(lr.pBits) + y * lr.Pitch);
                for (uint32_t x = 0; x < t.width; ++x) {
                    auto p = &t.rgba[(y * t.width + x) * 4];
                    row[x] = (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
                }
            }
            atlas->UnlockRect(0);
        }
        return true;
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

    bool mesh(SCRenderAPI::Mesh m, SCRenderAPI::Context context) override {
        // Normal before-HUD composition is permitted only with the captured depth.
        if (context.phase == SCRenderAPI::Phase::BeforeHUD && !context.depthAvailable)
            return false;
        return !m.triangles ||
               (m.vertices &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m.triangles, m.vertices,
                                                  sizeof(SCRenderAPI::Vertex))));
    }
};
