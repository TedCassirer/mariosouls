#pragma once
// Appearance survives native ownership of spawning, streaming and scripted work.
// This path never captures the body, submits motion, filters controls or attacks.
static SCSession::Identity presentationIdentity{};
static Vec presentationPrevious{};
static DWORD presentationTime = 0, presentationRetry = 0;

static void presentationRelease() {
    if (!presentationActive)
        return;
    visibilityRelease();
    if (visibilityIdentity(presentationIdentity.actor) && hideNative)
        put(presentationIdentity.actor + 0x258, savedOpacity);
    presentationActive = false;
    presentationIdentity = {};
    if (character >= 0) {
        guest->destroy(character);
        character = -1;
    }
    geo.numTrianglesUsed = 0;
}

static void presentationValidate() {
    SCSession::Identity next = {player, posData, read<uint32_t>(player + 0x28), currentMap()};
    if (presentationActive && (!next.valid() || !(presentationIdentity == next) || !cam))
        presentationRelease();
}

static void presentationTick() {
    if (characterOwned())
        return;
    SCSession::Identity next = {player, posData, read<uint32_t>(player + 0x28), currentMap()};
    Vec host{};
    float yaw = 0;
    bool valid = ready && characterSession.requested() && next.valid() && cam &&
                 get(posData + 0x10, host) && isfinite(dot(host, host)) && get(posData + 4, yaw) &&
                 isfinite(yaw);
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
        lastPlayer = player;
        savedOpacity = read<float>(player + 0x258);
        hideNative = (classSelection.required() ||
                      GetPrivateProfileIntA("MarioSouls", "HideNative", 0, ini) != 0) &&
                     isfinite(savedOpacity) && savedOpacity >= 0 && savedOpacity <= 1;
        presentationActive = true;
        logf("PRESENTATION native-driven actor=%08x map=%d hp=%d", unsigned(player), next.map,
             read<int>(player + 0x2d4));
    }
    Vec delta = host - presentationPrevious;
    if (DWORD(now - presentationTime) < 33 && geo.numTrianglesUsed) {
        // Keep the mesh registered to the moving native actor between animation ticks.
        origin = origin + delta;
        presentationPrevious = host;
        if (hideNative)
            put(player + 0x258, 0.f);
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
    if (!guestPose(character, &pose, &state, &geo) || !geo.numTrianglesUsed) {
        logf("PRESENTATION pose unavailable; retry pending");
        presentationRelease();
        presentationRetry = now + 1000;
        return;
    }
    state.flags = 0;
    state.attackId = state.interruptedAttackId = -1;
    state.landingRecoveryTicks = 0;
    // Only hide the human once a validated replacement mesh exists.
    if (!characterVisibility.armed)
        visibilityCapture();
    if (hideNative)
        put(player + 0x258, 0.f);
}
