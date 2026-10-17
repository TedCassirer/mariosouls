#pragma once

// Appearance survives native ownership of spawning, streaming and scripted work.
// This path never captures the body, submits motion, filters controls or attacks.

static void presentationRelease() {
    if (!characterSession.presentation.active)
        return;
    visualReset();
    visibilityRelease();
    characterSession.presentation.active = false;
    characterSession.presentation.identity = {};
    if (characterSession.instance.id >= 0) {
        adapter.api->destroy(characterSession.instance.id);
        characterSession.instance.id = -1;
    }
    adapter.mesh.view.numTrianglesUsed = 0;
}

static void presentationValidate() {
    SCSession::Identity next = {player, playerPhysics, read<uint32_t>(player + 0x28), currentMap()};
    if (characterSession.presentation.active &&
        (!next.valid() || !(characterSession.presentation.identity == next) || !playerCamera))
        presentationRelease();
}

static void presentationTick() {
    if (characterOwned())
        return;
    SCSession::Identity next = {player, playerPhysics, read<uint32_t>(player + 0x28), currentMap()};
    Vec host{};
    float yaw = 0;
    bool valid = ready && hostPermits(SCHost::Operation::Presentation) &&
                 characterSession.control.session.requested() && next.valid() && playerCamera &&
                 get(playerPhysics + 0x10, host) && isfinite(dot(host, host)) &&
                 get(playerPhysics + 4, yaw) && isfinite(yaw);
    if (!valid) {
        presentationRelease();
        return;
    }
    DWORD now = GetTickCount();
    if (!characterSession.presentation.active) {
        if (characterSession.presentation.retry &&
            int32_t(now - characterSession.presentation.retry) < 0)
            return;
        classPreviewRelease();
        characterSession.instance.origin = host;
        if (loadMap())
            rebuildSurfaces();
        else
            adapter.api->surfaces(nullptr, 0, 0);
        adapter.api->surfaces(nullptr, 0, 1);
        characterSession.instance.id = adapter.api->create(0, 0, 0, yaw - 3.14159265f);
        if (characterSession.instance.id < 0) {
            characterSession.presentation.retry = now + 1000;
            return;
        }
        characterSession.presentation.identity = next;
        characterSession.presentation.previous = host;
        characterSession.presentation.time = now - 34;
        characterSession.native.actor = player;
        characterSession.native.hide =
            classSelection.required() ||
            GetPrivateProfileIntA("MarioSouls", "HideNative", 0, ini) != 0;
        if (characterSession.native.hide && !hostPermits(SCHost::Operation::Replacement)) {
            adapter.api->destroy(characterSession.instance.id);
            characterSession.instance.id = -1;
            characterSession.presentation.retry = now + 1000;
            return;
        }
        characterSession.presentation.active = true;
    }
    Vec delta = host - characterSession.presentation.previous;
    if (DWORD(now - characterSession.presentation.time) < 33 &&
        adapter.mesh.view.numTrianglesUsed) {
        // Keep the mesh registered to the moving native actor between animation ticks.
        characterSession.instance.origin = characterSession.instance.origin + delta;
        characterSession.presentation.previous = host;
        return;
    }
    // Rebase presentation geometry without consulting old map floors.
    characterSession.instance.origin = host;
    characterSession.presentation.previous = host;
    characterSession.presentation.time = now;
    SC_Pose pose = {};
    pose.phase = -1;
    pose.yaw = yaw - 3.14159265f;
    bool dead = read<int>(player + 0x2d4) <= 0;
    auto o = nativeInteractionState();
    pose.kind = dead && (adapter.profile->capabilities & SC_CAP_DEATH_POSE) ? SC_POSE_DEATH
                : o.ladder                                                  ? SC_POSE_CLIMB
                : o.animation >= 7700 && o.animation < 7750                 ? SC_POSE_REST
                : o.animation == 7698                                       ? SC_POSE_DOWNED
                : dot(delta, delta) > .00001f                               ? SC_POSE_WALK
                                                                            : SC_POSE_NEUTRAL;
    if (pose.kind == SC_POSE_REST)
        pose.phase = 1;
    if (!guestPose(characterSession.instance.id, &pose, &characterSession.instance.state,
                   &adapter.mesh.view) ||
        !adapter.mesh.view.numTrianglesUsed) {
        logMessage("PRESENTATION pose unavailable; retry pending");
        presentationRelease();
        characterSession.presentation.retry = now + 1000;
        return;
    }
    SCState::clearCombat(characterSession.instance.state);
    // Only hide the human once a validated replacement mesh exists.
    if (!characterVisibility.armed)
        visibilityCapture();
}
