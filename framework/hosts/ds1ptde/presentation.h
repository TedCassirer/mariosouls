#pragma once
// Appearance survives native ownership of spawning, streaming and scripted work.
// This path never captures the body, submits motion, filters controls or attacks.
static SCSession::Identity presentationIdentity{};
static Vec presentationPrevious{};
static DWORD presentationTime = 0, presentationRetry = 0;

static void presentationRelease() {
    if (!presentationActive)
        return;
    visualReset();
    visibilityRelease();
    presentationActive = false;
    presentationIdentity = {};
    if (character >= 0) {
        guest->destroy(character);
        character = -1;
    }
    characterMesh.view.numTrianglesUsed = 0;
}

static void presentationValidate() {
    SCSession::Identity next = {player, playerPhysics, read<uint32_t>(player + 0x28), currentMap()};
    if (presentationActive && (!next.valid() || !(presentationIdentity == next) || !playerCamera))
        presentationRelease();
}

static void presentationTick() {
    if (characterOwned())
        return;
    SCSession::Identity next = {player, playerPhysics, read<uint32_t>(player + 0x28), currentMap()};
    Vec host{};
    float yaw = 0;
    bool valid = ready && characterControl.session.requested() && next.valid() && playerCamera &&
                 get(playerPhysics + 0x10, host) && isfinite(dot(host, host)) &&
                 get(playerPhysics + 4, yaw) && isfinite(yaw);
    if (!valid) {
        presentationRelease();
        return;
    }
    DWORD now = GetTickCount();
    if (!presentationActive) {
        if (presentationRetry && int32_t(now - presentationRetry) < 0)
            return;
        classPreviewRelease();
        origin = host;
        if (loadMap())
            rebuildSurfaces();
        else
            guest->surfaces(nullptr, 0, 0);
        guest->surfaces(nullptr, 0, 1);
        character = guest->create(0, 0, 0, yaw - 3.14159265f);
        if (character < 0) {
            presentationRetry = now + 1000;
            return;
        }
        presentationIdentity = next;
        presentationPrevious = host;
        presentationTime = now - 34;
        ownedActor = player;
        hideNative = classSelection.required() ||
                     GetPrivateProfileIntA("MarioSouls", "HideNative", 0, ini) != 0;
        if (hideNative && !characterVisibilityReady) {
            guest->destroy(character);
            character = -1;
            presentationRetry = now + 1000;
            return;
        }
        presentationActive = true;
    }
    Vec delta = host - presentationPrevious;
    if (DWORD(now - presentationTime) < 33 && characterMesh.view.numTrianglesUsed) {
        // Keep the mesh registered to the moving native actor between animation ticks.
        origin = origin + delta;
        presentationPrevious = host;
        return;
    }
    // Rebase presentation geometry without consulting old map floors.
    origin = host;
    presentationPrevious = host;
    presentationTime = now;
    SC_Pose pose = {};
    pose.phase = -1;
    pose.yaw = yaw - 3.14159265f;
    bool dead = read<int>(player + 0x2d4) <= 0;
    auto o = nativeInteractionState();
    pose.kind = dead && (profile->capabilities & SC_CAP_DEATH_POSE) ? SC_POSE_DEATH
                : o.ladder                                          ? SC_POSE_CLIMB
                : o.animation >= 7700 && o.animation < 7750         ? SC_POSE_REST
                : o.animation == 7698                               ? SC_POSE_DOWNED
                : dot(delta, delta) > .00001f                       ? SC_POSE_WALK
                                                                    : SC_POSE_NEUTRAL;
    if (pose.kind == SC_POSE_REST)
        pose.phase = 1;
    if (!guestPose(character, &pose, &state, &characterMesh.view) ||
        !characterMesh.view.numTrianglesUsed) {
        logMessage("PRESENTATION pose unavailable; retry pending");
        presentationRelease();
        presentationRetry = now + 1000;
        return;
    }
    SCState::clearCombat(state);
    // Only hide the human once a validated replacement mesh exists.
    if (!characterVisibility.armed)
        visibilityCapture();
}
