#pragma once
#include "../../core/character_presentation.h"

// PTDE observation phase: native identities, preview/cinematic selection, lens
// defaults and authoritative motion correction. Borrowed data is valid only
// for this synchronous draw; no observation escapes into a queued GPU frame.
struct PTDECharacterView {
    const HostCharacter::MeshBuffers *mesh = nullptr;
    const SC_State *pose = nullptr;
    SCCharacterPresentation::CameraInput lens{};
    SCRenderAPI::Camera camera{};
    Vec origin{};
    float scale = 1;
    const float *modelWorld = nullptr;
    bool cinematic = false, drawCharacter = false;
};

static bool observeCharacterView(bool preview, SCRenderAPI::Viewport vp, PTDECharacterView &out) {
    bool cinematic = !preview && cutsceneVisible();
    if (cinematic) {
        auto current = PTDECutscene::observe(cutsceneVerified);
        if (!current.usable || !current.visible || !current.same(cutsceneFrame))
            return false;
        cutsceneFrame = current; // Camera/root refreshed at draw, never interpolated across cuts.
    }
    const auto &mesh = cinematic ? cutsceneMesh : adapter.mesh;
    const auto &poseState = cinematic ? cutsceneState : characterSession.instance.state;
    bool drawCharacter =
        (cinematic || preview || (!cutsceneFrame.active && characterPresented())) &&
        mesh.view.numTrianglesUsed;
    if (!preview && !cinematic &&
        (!hostPermits(SCHost::Operation::Presentation) ||
         chain(gameSlots.player, {0, 4, 0}) != player ||
         chain(player + 0x28, {0, 0x1c}) != playerPhysics))
        return false;
    Vec eye = (preview || cinematic) ? Vec{} : cameraEye();
    Vec at = eye + ((preview || cinematic) ? Vec{0, 0, 1} : cameraForward());
    if (preview) {
        float height = classProfile.previewHeightCm * .01f;
        eye = {0, height * .55f, -height * 2.f};
        at = {0, height * .5f, 0};
    }
    float fov = read<float>(playerCamera + 0x50), aspect = read<float>(playerCamera + 0x54),
          zn = read<float>(playerCamera + 0x58), zf = read<float>(playerCamera + 0x5c);
    if (!isfinite(fov) || fov < .1f || fov > 2.8f)
        fov = .75049f;
    if (!isfinite(aspect) || aspect < .5f || aspect > 5)
        aspect = vp.width / vp.height;
    if (!isfinite(zn) || zn < .001f || zn > 2)
        zn = .05f;
    if (!isfinite(zf) || zf < 50 || zf > 10000)
        zf = 3000;
    if (preview) {
        fov = .65f;
        aspect = classPreviewAspect;
        zn = .01f;
        zf = 100;
    }
    if (cinematic) {
        fov = cutsceneFrame.fov;
    }
    Vec renderOrigin = (preview || cinematic) ? Vec{} : characterSession.instance.origin;
    float modelScale = preview ? 1.f : characterBody.modelScale;
    if (!preview && !cinematic && characterOwned() && worldMode == 1 && worldMotionReady) {
        Vec offset =
            read<Vec>(playerPhysics + 0x10) -
            (characterSession.instance.origin +
             Vec{poseState.position[0], poseState.position[1], poseState.position[2]} * .01f);
        if (isfinite(dot(offset, offset)) && dot(offset, offset) < 4)
            renderOrigin = renderOrigin + offset;
    }
    double renderTime = visualTime();
    characterSession.visual.alpha = characterSession.visual.mesh.alpha(renderTime);
    if (!preview && !cinematic && drawCharacter) {
        Vec anchor =
            renderOrigin +
            Vec{poseState.position[0], poseState.position[1], poseState.position[2]} * .01f;
        float p[3] = {anchor.x, anchor.y, anchor.z};
        characterSession.visual.anchor.observe(
            p, renderTime, uint64_t(player) << 32 | uint32_t(currentMap()),
            !characterSession.visual.focused ||
                SCFeedback::pending(feedbackPause.until, GetTickCount()));
        renderOrigin =
            renderOrigin + Vec{characterSession.visual.anchor.sample(0, renderTime) - p[0],
                               characterSession.visual.anchor.sample(1, renderTime) - p[1],
                               characterSession.visual.anchor.sample(2, renderTime) - p[2]};
    }

    out.mesh = &mesh;
    out.pose = &poseState;
    out.origin = renderOrigin;
    out.scale = modelScale;
    out.cinematic = cinematic;
    out.drawCharacter = drawCharacter;
    out.modelWorld = cinematic ? cutsceneFrame.model : nullptr;
    out.lens = {{eye.x, eye.y, eye.z},
                {at.x, at.y, at.z},
                fov,
                aspect,
                zn,
                zf,
                cinematic ? cutsceneFrame.camera : nullptr};
    return SCCharacterPresentation::camera(out.lens, out.camera);
}
