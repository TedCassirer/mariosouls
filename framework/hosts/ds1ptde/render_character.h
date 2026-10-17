#pragma once

// Host implementation fragment: uses the validated presentation state and D3D9 backend.
// Builds render-only vertices; never advances guest or native simulation.

static D3DMATRIX identity() {
    D3DMATRIX m = {};
    m._11 = m._22 = m._33 = m._44 = 1;
    return m;
}

static D3DMATRIX viewMatrix(Vec eye, Vec at) {
    Vec z = unit(at - eye), x = unit(cross({0, 1, 0}, z)), y = cross(z, x);
    D3DMATRIX m = {};
    m._11 = x.x;
    m._12 = y.x;
    m._13 = z.x;
    m._21 = x.y;
    m._22 = y.y;
    m._23 = z.y;
    m._31 = x.z;
    m._32 = y.z;
    m._33 = z.z;
    m._41 = -dot(x, eye);
    m._42 = -dot(y, eye);
    m._43 = -dot(z, eye);
    m._44 = 1;
    return m;
}

using Vertex = SCRenderAPI::Vertex;

#include "launcher_render.h"

static bool draw(IDirect3DDevice9 *d, bool preview = false,
                 SCRenderAPI::Phase phase = SCRenderAPI::Phase::Scene) {
    if (preview ? !classPreviewActive : !renderWanted())
        return false;
    bool cinematic = !preview && cutsceneVisible();
    if (cinematic) {
        auto current = PTDECutscene::observe(cutsceneVerified);
        if (!current.usable || !current.visible || !current.same(cutsceneFrame))
            return false;
        cutsceneFrame = current; // Camera/root refreshed at draw, never interpolated across cuts.
    }
    const auto &mesh = cinematic ? cutsceneMesh : characterMesh;
    const auto &poseState = cinematic ? cutsceneState : state;
    bool drawCharacter =
        (cinematic || preview || (!cutsceneFrame.active && characterPresented())) &&
        mesh.view.numTrianglesUsed;
    PTDERenderer renderer(d, tex);
    if (!renderer.texture(
            {characterMesh.atlasPixels.data(), profile->textureWidth, profile->textureHeight}))
        return false;
    IDirect3DStateBlock9 *saved = nullptr;
    if (FAILED(d->CreateStateBlock(D3DSBT_ALL, &saved)))
        return false;
    D3DVIEWPORT9 vp;
    d->GetViewport(&vp);
    Vec eye = cameraEye();
    Vec at = eye + cameraForward();
    if (preview) {
        float height = classProfile.previewHeightCm * .01f;
        eye = {0, height * .55f, -height * 2.f};
        at = {0, height * .5f, 0};
    }
    D3DMATRIX world = identity(), view = viewMatrix(eye, at), proj = {};
    float fov = read<float>(playerCamera + 0x50), aspect = read<float>(playerCamera + 0x54),
          zn = read<float>(playerCamera + 0x58), zf = read<float>(playerCamera + 0x5c);
    if (fov < .1f || fov > 2.8f)
        fov = .75049f;
    if (aspect < .5f || aspect > 5)
        aspect = float(vp.Width) / vp.Height;
    if (zn < .001f || zn > 2)
        zn = .05f;
    if (zf < 50 || zf > 10000)
        zf = 3000;
    if (preview) {
        fov = .65f;
        aspect = classPreviewAspect;
        zn = .01f;
        zf = 100;
    }
    if (cinematic) {
        SCPresentation::view(cutsceneFrame.camera, reinterpret_cast<float *>(&view));
        fov = cutsceneFrame.fov;
    }
    float ys = 1 / tanf(fov / 2);
    proj._11 = ys / aspect;
    proj._22 = ys;
    proj._33 = zf / (zf - zn);
    proj._34 = 1;
    proj._43 = -zn * proj._33;
    Vec renderOrigin = (preview || cinematic) ? Vec{} : origin;
    float modelScale = preview ? 1.f : characterBody.modelScale;
    if (!preview && !cinematic && characterOwned() && worldMode == 1 && worldMotionReady) {
        Vec offset =
            read<Vec>(playerPhysics + 0x10) -
            (origin +
             Vec{poseState.position[0], poseState.position[1], poseState.position[2]} * .01f);
        if (isfinite(dot(offset, offset)) && dot(offset, offset) < 4)
            renderOrigin = renderOrigin + offset;
    }
    double renderTime = visualTime();
    visualAlpha = visualMesh.alpha(renderTime);
    if (!preview && !cinematic && drawCharacter) {
        Vec anchor =
            renderOrigin +
            Vec{poseState.position[0], poseState.position[1], poseState.position[2]} * .01f;
        float p[3] = {anchor.x, anchor.y, anchor.z};
        visualAnchor.observe(p, renderTime, uint64_t(player) << 32 | uint32_t(currentMap()),
                             !visualFocused ||
                                 SCFeedback::pending(feedbackPause.until, GetTickCount()));
        renderOrigin = renderOrigin + Vec{visualAnchor.sample(0, renderTime) - p[0],
                                          visualAnchor.sample(1, renderTime) - p[1],
                                          visualAnchor.sample(2, renderTime) - p[2]};
    }
    bool blendedMesh = !cinematic && visualMesh.matches(mesh.view);
    std::vector<Vertex> verts(drawCharacter ? mesh.view.numTrianglesUsed * 3 : 0);
    for (size_t i = 0; i < verts.size(); i++) {
        auto &p = verts[i];
        p.x = renderOrigin.x +
              SCBody::modelCoordinate(blendedMesh ? poseState.position[0] +
                                                        visualMesh.position(i * 3, visualAlpha)
                                                  : mesh.positions[i * 3],
                                      poseState.position[0], modelScale) *
                  .01f;
        p.y = renderOrigin.y +
              SCBody::modelCoordinate(blendedMesh ? poseState.position[1] +
                                                        visualMesh.position(i * 3 + 1, visualAlpha)
                                                  : mesh.positions[i * 3 + 1],
                                      poseState.position[1], modelScale) *
                  .01f;
        p.z = renderOrigin.z +
              SCBody::modelCoordinate(blendedMesh ? poseState.position[2] +
                                                        visualMesh.position(i * 3 + 2, visualAlpha)
                                                  : mesh.positions[i * 3 + 2],
                                      poseState.position[2], modelScale) *
                  .01f;
        int r = std::clamp(int(mesh.colors[i * 3] * 255), 0, 255),
            g = std::clamp(int(mesh.colors[i * 3 + 1] * 255), 0, 255),
            b = std::clamp(int(mesh.colors[i * 3 + 2] * 255), 0, 255);
        p.color = 0xff000000 | (r << 16) | (g << 8) | b;
        Vec normal = unit(
            {blendedMesh ? visualMesh.normal(i * 3, visualAlpha) : mesh.normals[i * 3],
             blendedMesh ? visualMesh.normal(i * 3 + 1, visualAlpha) : mesh.normals[i * 3 + 1],
             blendedMesh ? visualMesh.normal(i * 3 + 2, visualAlpha) : mesh.normals[i * 3 + 2]});
        int light = int(255 * (.4f + .6f * std::max(0.f, dot(normal, unit({-.3f, .8f, -.5f})))));
        p.shade = 0xff000000 | (light << 16) | (light << 8) | light;
        if (cinematic) {
            float point[3] = {p.x, p.y, p.z}, worldPoint[3], n[3] = {normal.x, normal.y, normal.z},
                  worldNormal[3];
            SCPresentation::transform(cutsceneFrame.model, point, worldPoint, true);
            SCPresentation::transform(cutsceneFrame.model, n, worldNormal, false);
            p.x = worldPoint[0];
            p.y = worldPoint[1];
            p.z = worldPoint[2];
            int lit = int(255 * (.4f + .6f * std::max(0.f, dot(unit({worldNormal[0], worldNormal[1],
                                                                     worldNormal[2]}),
                                                               unit({-.3f, .8f, -.5f})))));
            p.shade = 0xff000000 | (lit << 16) | (lit << 8) | lit;
        }
        p.u = mesh.uvs[i * 2];
        p.v = mesh.uvs[i * 2 + 1];
    }
    d->SetVertexShader(NULL);
    d->SetPixelShader(NULL);
    d->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    SCRenderAPI::Camera renderCamera{};
    memcpy(renderCamera.world, &world, sizeof(world));
    memcpy(renderCamera.view, &view, sizeof(view));
    memcpy(renderCamera.projection, &proj, sizeof(proj));
    renderer.camera(renderCamera);
    d->SetRenderState(D3DRS_LIGHTING, FALSE);
    d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d->SetRenderState(D3DRS_ZENABLE, (preview || depthTest) ? TRUE : FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    d->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE,
                      (profile->capabilities & SC_CAP_ALPHA_TEST) ? TRUE : FALSE);
    d->SetRenderState(D3DRS_ALPHAREF, 127);
    d->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    d->SetRenderState(D3DRS_FOGENABLE, FALSE);
    d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    d->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    d->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    d->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    d->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    d->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    d->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    d->SetTexture(0, tex);
    d->SetTextureStageState(0, D3DTSS_COLOROP,
                            (profile->capabilities & SC_CAP_TEXTURE_MODULATE)
                                ? D3DTOP_MODULATE
                                : D3DTOP_BLENDTEXTUREALPHA);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1,
                            (profile->capabilities & SC_CAP_ALPHA_TEST) ? D3DTA_TEXTURE
                                                                        : D3DTA_DIFFUSE);
    d->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
    d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_SPECULAR);
    d->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
    d->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    d->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    d->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    auto renderPhase = preview ? SCRenderAPI::Phase::Preview : phase;
    HRESULT hr = renderer.mesh({verts.data(), drawCharacter ? mesh.view.numTrianglesUsed : 0},
                               {renderPhase, sceneDepth != nullptr})
                     ? S_OK
                     : E_FAIL;
    static bool logged = false;
    if (!logged) {
        logMessage("draw result=%08lx viewport=%ux%u fov=%.5f near=%.5f far=%.1f", hr, vp.Width,
                   vp.Height, fov, zn, zf);
        logged = true;
    }
    if (cinematic && phase == SCRenderAPI::Phase::Scene)
        cutsceneRendered = SUCCEEDED(hr);
    if (!preview && !cinematic) {
        if (drawCharacter) {
            equipmentDraw(d, renderOrigin);
            feedbackDraw(d);
        }
        debugDraw(d);
        launcherDraw(d);
    }
    saved->Apply();
    saved->Release();
    return SUCCEEDED(hr);
}
