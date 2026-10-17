#pragma once
#include "native_cutscene.h"
// Presentation only. No cutscene playback/event/inventory/control flags are written.
static bool cutsceneVerified = false, cutsceneRendered = false;
static PTDECutscene::Frame cutsceneFrame, cutsceneLease;
static HostCharacter::MeshBuffers cutsceneMesh;
static SC_State cutsceneState{};
static int cutsceneCharacter = -1;
static DWORD cutscenePoseTime = 0, cutsceneRetry = 0;
static uint8_t cutsceneBodyBit = 0, cutsceneAssemblyBit = 0;
static bool cutsceneHidden = false;

static void cutsceneUnhide(const PTDECutscene::Frame &current) {
    // Never write saved model pointers after a cut/model has been destroyed or
    // replaced. Native cut teardown owns their destruction and visibility.
    if (cutsceneHidden && current.active && current.part && current.same(cutsceneLease)) {
        if (current.body)
            visibilityBit(current.body, 0x76, 0x68, cutsceneBodyBit != 0);
        if (current.assembly)
            visibilityBit(current.assembly, 0x92, 0x84, cutsceneAssemblyBit != 0);
    }
    cutsceneHidden = false;
    cutsceneLease = {};
}

static void cutsceneRelease() {
    cutsceneUnhide(cutsceneFrame);
    if (cutsceneCharacter >= 0)
        adapter.api->destroy(cutsceneCharacter);
    cutsceneCharacter = -1;
    cutsceneMesh.view.numTrianglesUsed = 0;
    cutsceneRendered = false;
}

static bool cutsceneVisible() {
    return cutsceneFrame.active && cutsceneFrame.usable && cutsceneFrame.visible &&
           cutsceneCharacter >= 0 && cutsceneMesh.view.numTrianglesUsed &&
           characterSession.control.session.requested();
}

static void cutsceneObserve() {
    auto current = PTDECutscene::observe(cutsceneVerified);
    if (!current.same(cutsceneFrame)) {
        cutsceneUnhide(current);
        if (cutsceneCharacter >= 0)
            adapter.api->destroy(cutsceneCharacter);
        cutsceneCharacter = -1;
        cutsceneMesh.view.numTrianglesUsed = 0;
        cutsceneRendered = false;
        cutsceneRetry = 0;
    }
    cutsceneFrame = current;
}

static void cutsceneTick() {
    if (!ready || !characterSession.control.session.requested() || !cutsceneFrame.active ||
        !cutsceneFrame.usable || !cutsceneFrame.visible) {
        cutsceneRelease();
        return;
    }
    DWORD now = GetTickCount();
    if (cutsceneCharacter < 0) {
        if (cutsceneRetry && int32_t(now - cutsceneRetry) < 0)
            return;
        classPreviewRelease();
        cutsceneMesh.allocate(*adapter.profile);
        cutsceneCharacter = adapter.api->create(0, 0, 0, -3.14159265f);
        cutscenePoseTime = GetTickCount() - 34;
    }
    if (cutsceneCharacter < 0) {
        cutsceneRetry = now + 1000;
        return;
    }
    if (DWORD(now - cutscenePoseTime) >= 33) {
        SC_Pose pose{};
        pose.kind = SC_POSE_NEUTRAL;
        pose.phase = -1;
        pose.yaw = -3.14159265f;
        if (!adapter.api->pose(cutsceneCharacter, &pose, &cutsceneState, &cutsceneMesh.view) ||
            !cutsceneMesh.view.numTrianglesUsed) {
            cutsceneRelease();
            cutsceneRetry = now + 1000;
            return;
        }
        cutsceneState.flags = 0;
        cutsceneState.attackId = cutsceneState.interruptedAttackId = -1;
        cutscenePoseTime = now;
    }
    // Confirm a successful composite before hiding a native copy. If rendering
    // stops, restore it rather than leaving an invisible player in the scene.
    if (cutsceneRendered && !cutsceneHidden) {
        uint8_t body = 0, assembly = 0;
        bool readable = (!cutsceneFrame.body || get(cutsceneFrame.body + 0x76, body)) &&
                        (!cutsceneFrame.assembly || get(cutsceneFrame.assembly + 0x92, assembly));
        if (readable) {
            cutsceneBodyBit = body & 1;
            cutsceneAssemblyBit = assembly & 1;
            cutsceneLease = cutsceneFrame;
            cutsceneHidden = true;
        }
    }
    if (!cutsceneRendered)
        cutsceneUnhide(cutsceneFrame);
    if (cutsceneHidden) {
        bool hidden =
            (!cutsceneFrame.body || visibilityBit(cutsceneFrame.body, 0x76, 0x68, false)) &&
            (!cutsceneFrame.assembly || visibilityBit(cutsceneFrame.assembly, 0x92, 0x84, false));
        if (!hidden)
            cutsceneUnhide(cutsceneFrame);
    }
    cutsceneRendered = false;
}
