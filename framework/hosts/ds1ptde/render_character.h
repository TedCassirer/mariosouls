#pragma once
#include "render_observation.h"
#include "launcher_render.h"

static bool renderWanted() {
    return cutsceneVisible() || (characterPresented() && adapter.mesh.view.numTrianglesUsed) ||
           debugVisible() || launcherVisible();
}

// Composition only. Observations and reusable vertex assembly have separate
// inputs; the backend owns the graphics-state lease and all device resources.
static bool draw(IDirect3DDevice9 *d, bool preview = false,
                 SCRenderAPI::Phase phase = SCRenderAPI::Phase::Scene) {
    if (preview ? !classPreviewActive : !renderWanted())
        return false;
    PTDERenderer renderer(d);
    if (!renderer.texture({adapter.mesh.atlasPixels.data(), adapter.profile->textureWidth,
                           adapter.profile->textureHeight}))
        return false;
    PTDEGraphics::StateScope saved(d);
    if (!saved.valid())
        return false;
    auto vp = renderer.viewport();
    PTDECharacterView frame;
    if (!observeCharacterView(preview, vp, frame))
        return false;
    std::vector<SCRenderAPI::Vertex> verts;
    if (frame.drawCharacter)
        SCCharacterPresentation::mesh({frame.mesh->view,
                                       frame.pose->position,
                                       {frame.origin.x, frame.origin.y, frame.origin.z},
                                       frame.scale,
                                       frame.cinematic ? nullptr : &characterSession.visual.mesh,
                                       characterSession.visual.alpha,
                                       frame.modelWorld},
                                      verts);
    renderer.camera(frame.camera);
    SCRenderAPI::Material style{};
    style.image = {adapter.mesh.atlasPixels.data(), adapter.profile->textureWidth,
                   adapter.profile->textureHeight};
    style.color = (adapter.profile->capabilities & SC_CAP_TEXTURE_MODULATE)
                      ? SCRenderAPI::ColorMode::Modulate
                      : SCRenderAPI::ColorMode::TextureAlpha;
    style.shade = true;
    style.depthTest = preview || depthTest;
    style.alphaCutoff = (adapter.profile->capabilities & SC_CAP_ALPHA_TEST) ? 127 : 0;
    auto renderPhase = preview ? SCRenderAPI::Phase::Preview : phase;
    SCRenderAPI::Context context{renderPhase, sceneDepth != nullptr};
    HRESULT hr =
        renderer.mesh({verts.data(), frame.drawCharacter ? frame.mesh->view.numTrianglesUsed : 0},
                      style, context)
            ? S_OK
            : E_FAIL;
    static bool logged = false;
    if (!logged) {
        logMessage("draw result=%08lx viewport=%ux%u fov=%.5f near=%.5f far=%.1f", hr,
                   unsigned(vp.width), unsigned(vp.height), frame.lens.fov, frame.lens.nearPlane,
                   frame.lens.farPlane);
        logged = true;
    }
    if (frame.cinematic && phase == SCRenderAPI::Phase::Scene)
        cutsceneRendered = SUCCEEDED(hr);
    if (!preview && !frame.cinematic) {
        if (frame.drawCharacter) {
            equipmentDraw(renderer, frame.origin, context);
            feedbackDraw(renderer, context);
        }
        debugDraw(renderer, context);
        launcherDraw(renderer, context);
    }
    return SUCCEEDED(hr);
}
