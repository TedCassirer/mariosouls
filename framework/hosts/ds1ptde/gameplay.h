#pragma once

// Host implementation fragment, included after action/combat/presentation modules.
// Owns frame observation, lifecycle, input routing and fixed-rate simulation.

// One render-frame observation is shared by every phase. Input routing may
// consume jump/interact edges, so later simulation sees the routed controller.
struct GameplayFrame {
    bool focus;
    DWORD inputTime;
    SCController::Sample pad;
    SCController::Input controller;
    bool padToggle;
    SCSession::Identity identity;
};

static GameplayFrame pollGameplayFrame() {
    player = chain(gameSlots.player, {0, 4, 0});
    playerCamera = chain(gameSlots.camera, {0, 0x3c, 0x60});
    uintptr_t mp = read<uint32_t>(player + 0x28);
    playerPhysics = read<uint32_t>(mp + 0x1c);
    presentationValidate();
    if (!cutsceneFrame.active)
        classTick();
    if (characterOwned() && !characterSession.control.session.requested())
        releaseControl(false);
    DWORD fg = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &fg);
    bool focus = fg == GetCurrentProcessId();
    characterSession.visual.focused = focus;
    debugInput(focus);
    feedbackFocused = focus;
    if (!focus)
        feedbackReset();
    static SCController::Toggle controllerToggle;
    DWORD inputTime = GetTickCount();
    auto pad = HostController::poll(inputTime);
    auto controller = pad.connected && focus ? SCController::map(pad.pad) : SCController::Input{};
    static int lastController = -2;
    if (pad.slot != lastController) {
        logMessage("controller %s slot=%d", pad.connected ? "connected" : "disconnected", pad.slot);
        lastController = pad.slot;
    }

    bool padToggle = controllerToggle.update(pad, focus, inputTime);
    SCSession::Identity identity = {player, playerPhysics, mp, currentMap()};
    return {focus, inputTime, pad, controller, padToggle, identity};
}

// Resolve map/actor replacement and death before any input reaches an adapter.
static bool updateCharacterLifecycle(const GameplayFrame &frame, double &acc) {
    if (cutsceneFrame.active) {
        if (characterPresented())
            releaseControl(true);
        acc = 0;
        return false;
    }
    if (SCSession::transition(characterSession.control.session, frame.identity, playerCamera != 0,
                              !nativeReaction && !equipmentActionOwned) ==
        SCSession::Transition::SeamlessMap) {
        if (loadMap() && characterSession.control.session.changeMap(frame.identity)) {
            rebuildSurfaces();
            worldActivate();
        }
    }
    if (characterOwned() &&
        SCSession::transition(characterSession.control.session, frame.identity, playerCamera != 0,
                              false) == SCSession::Transition::ReplaceActor) {
        logMessage("SESSION native actor changed; clearing pending work");
        releaseControl(true);
    }
    if (characterOwned() && read<int>(player + 0x2d4) <= 0 &&
        !characterSession.control.session.death()) {
        launcherCancel();
        if (!deathBegin(acc))
            releaseControl(true);
    }
    if (characterSession.control.session.death() && read<int>(player + 0x2d4) > 0) {
        releaseControl(true);
        acc = 0;
    }
    if ((frame.focus && pressed(VK_F6)) || frame.padToggle) {
        classSelection.manual(player);
        // A pending load request is not a visible character. Pressing the toggle
        // while still human should enable the guest, not cancel automatic activation.
        if (characterPresented()) {
            characterSession.control.session.request(false);
            releaseControl(false);
        } else {
            characterSession.control.session.request(true);
            if (!activate())
                characterSession.control.session.failed(frame.inputTime);
        }
    }
    cameraTick();
    if (!equipmentActionOwned || characterSession.control.session.death())
        projectileRateRelease();
    if (characterSession.control.session.death()) {
        deathTick(acc, frame.pad.slot);
        return false;
    }
    if (!characterOwned() && characterSession.control.session.requested()) {
        bool safe =
            frame.focus && frame.identity.valid() && playerCamera && read<int>(player + 0x2d4) > 0;
        if (safe) {
            auto o = nativeInteractionState();
            safe = o.valid && !o.menu && !o.event && !o.ladder &&
                   nativeActiveAllowed(player, false) == 1;
        }
        if (characterSession.control.session.canRetry(frame.identity, safe, frame.inputTime)) {
            if (!activate())
                characterSession.control.session.failed(frame.inputTime);
        }
    }
    return true;
}

// Contextual actions get first refusal on input (for example A at a bonfire).
static void routeGameplayActions(GameplayFrame &frame) {
    static bool previousCancel = false, previousItem = false, previousActionFocus = false;
    static int previousActionSlot = -1;
    bool yDown = frame.pad.connected && (frame.pad.pad.wButtons & XINPUT_GAMEPAD_Y),
         xDown = frame.pad.connected && (frame.pad.pad.wButtons & XINPUT_GAMEPAD_X);
    bool qPressed = pressed('Q'), ePressed = pressed('E');
    bool padEdgeReady = previousActionFocus && previousActionSlot == frame.pad.slot;
    bool padCancel = padEdgeReady && yDown && !previousCancel,
         padItem = padEdgeReady && xDown && !previousItem;
    bool interactPressed = frame.focus && previousActionFocus && qPressed,
         cancelPressed = frame.focus && padCancel,
         itemPressed = frame.focus && previousActionFocus && (padItem || ePressed);
    previousCancel = yDown;
    previousItem = xDown;
    previousActionFocus = frame.focus;
    previousActionSlot = frame.pad.slot;
    if (!frame.focus) {
        actionCancelPulse();
        equipmentReset();
    }
    if (characterOwned() && !nativeActionOwned() && !equipmentActionOwned) {
        auto native = nativeInteractionState();
        if (native.valid && (native.menu || native.ladder || native.event))
            beginInteraction(frame.pad.slot, false);
    }
    launcherObserve(frame.focus);
    if (launcherBusy() && frame.focus && frame.pad.connected)
        frame.controller = SCController::aim(frame.pad.pad);
    static SCContext::Button contextA;
    auto context =
        contextA.update(frame.controller.jump, frame.focus && frame.pad.connected, player,
                        frame.pad.slot, characterOwned() && !nativeActionOwned() && !nativeReaction,
                        !launcherBusy() && (actionEligible(1) || launcherTarget));
    InterlockedExchange(&actionBlockHeldA, context.blockNative);
    frame.controller.jump = context.jump;
    if (context.interact) {
        if (!launcherInteract())
            requestAction(1, true);
    } else if (launcherBusy() && (interactPressed || cancelPressed))
        launcherCancel();
    else if (interactPressed && launcherInteract()) {
    } else if (interactPressed) {
        if (characterOwned() && !nativeActionOwned())
            requestAction(1, false);
        else if (nativeActionOwned() && actionCurrentRoute() == ActionRoute::Menu)
            actionQueue(1, frame.inputTime, false);
    }
    if (itemPressed && !launcherBusy() && characterOwned() && !nativeActionOwned())
        requestAction(2, padItem);
    if (characterOwned() && !nativeActionOwned() && !actionPending() && actionRequestKind) {
        logMessage("Action request unanswered; Character retained");
        actionRequestKind = 0;
    }
}

static void refreshGameplaySettings() {
    static DWORD lastLog = 0;
    if (GetTickCount() - lastLog > 2000) {
        lastLog = GetTickCount();
        combatSettings();
        renderMode = GetPrivateProfileIntA("MarioSouls", "RenderMode", 1, ini);
        depthTest = GetPrivateProfileIntA("MarioSouls", "DepthTest", 1, ini);
        combatEnabled = GetPrivateProfileIntA("MarioSouls", "EnableCombat", 0, ini) != 0;
    }
}

// Native actions and reactions own their frames. Reconcile the native motion
// acknowledgement before evaluating damage or beginning another guest tick.
static bool prepareCharacterSimulation(const GameplayFrame &frame, double &acc) {
    HostController::setOwner((characterOwned() || nativeActionOwned()) ? frame.pad.slot : -1);
    if (!nativeActionOwned())
        InterlockedExchange(&actionRoute,
                            LONG(characterOwned() ? ActionRoute::Character : ActionRoute::Free));
    static bool filteredLogged = false;
    if (!filteredLogged && HostController::nativePollCount() > 0) {
        logMessage("controller native input filtering confirmed; right stick preserved");
        filteredLogged = true;
    }
    if (nativeActionOwned()) {
        updateInteraction(acc, frame.focus, frame.pad.slot);
        return false;
    }
    if (!characterOwned()) {
        acc = 0;
        return false;
    }
    if (!hostPermits(SCHost::Operation::Movement)) {
        logMessage("Movement facilities lost; native controls restored (mask=%x)",
                   hostCapabilities().available);
        releaseControl(true);
        acc = 0;
        return false;
    }
    if (arenaRefresh())
        rebuildSurfaces();

    environmentFrame = environmentObserve(frame.focus);
    if (equipmentActionsTick(acc, frame.focus))
        return false;
    if (nativeReactionTick(acc, frame.focus, !!(environmentFrame & SC_ENV_REACTION_PRIORITY)))
        return false;
    worldPause(!frame.focus);
    Vec host = read<Vec>(playerPhysics + 0x10),
        expected =
            characterSession.instance.origin + Vec{characterSession.instance.state.position[0],
                                                   characterSession.instance.state.position[1],
                                                   characterSession.instance.state.position[2]} *
                                                   .01f;
    if (dot(host - expected, host - expected) > 64) {
        logMessage("native warp/load detected");
        releaseControl(true);
        return false;
    }
    bool resolvedTick = false;
    if (!worldReconcile(resolvedTick)) {
        logMessage("WORLD invalid handoff; native controls restored");
        releaseControl();
        return false;
    }
    worldHostSurfaces();
    if (fabsf(characterSession.instance.state.position[0]) > 4000 ||
        fabsf(characterSession.instance.state.position[2]) > 4000 ||
        fabsf(characterSession.instance.state.position[1]) > 5000) {
        Vec shift = {characterSession.instance.state.position[0],
                     characterSession.instance.state.position[1],
                     characterSession.instance.state.position[2]};
        characterSession.instance.origin = characterSession.instance.origin + shift * .01f;
        rebuildSurfaces();
        adapter.api->setPosition(characterSession.instance.id, 0, 0, 0);
        for (size_t i = 0; i < adapter.mesh.view.numTrianglesUsed * 3; i++) {
            adapter.mesh.positions[i * 3] -= shift.x;
            adapter.mesh.positions[i * 3 + 1] -= shift.y;
            adapter.mesh.positions[i * 3 + 2] -= shift.z;
        }
        characterSession.instance.state.position[0] = characterSession.instance.state.position[1] =
            characterSession.instance.state.position[2] = 0;
        // Contact/support patches use the same local coordinates as static
        // terrain. Refresh them before another guest step after a rebase;
        // otherwise a long lift ride loses its floor for one simulation tick.
        worldHostSurfaces();
    }
    feedbackPoll();
    int hp = read<int>(player + 0x2d4);
    bool hurt = hp < lastHP && hp > 0;
    if (hurt) {
        feedbackPause.clear();
        combatReset();
        int lost = lastHP - hp;
        if (SCReaction::guestHurt(defenseReady, nativeActiveAllowed(), launcherBusy()))
            pendingNativeHurt =
                std::max(pendingNativeHurt, lost >= read<int>(player + 0x2d8) / 5 ? 2u : 1u);
    }
    lastHP = hp;
    if (resolvedTick && hazardObserve(read<Vec>(playerPhysics + 0x10))) {
        environmentKill(acc, "native_kill_surface");
        return false;
    }
    if (resolvedTick && !pendingNativeHurt) {
        if (!combatEnvironment(acc))
            return false;
        combat();
        combatObjects();
        equipmentContact();
        feedbackPoll();
    }
    return true;
}

// Advance the guest at its fixed rate and submit motion for the native host to
// resolve. The next frame consumes that acknowledgement before opening contacts.
static void simulateCharacter(const GameplayFrame &frame, double dt, double &acc) {
    SC_Input input = {};
    Vec look = cameraForward();
    input.camLookX = look.x;
    input.camLookZ = look.z;
    if (frame.focus) {
        input.moveX = ((GetAsyncKeyState('D') & 0x8000) ? 1.f : 0) -
                      ((GetAsyncKeyState('A') & 0x8000) ? 1.f : 0);
        input.moveY = ((GetAsyncKeyState('W') & 0x8000) ? 1.f : 0) -
                      ((GetAsyncKeyState('S') & 0x8000) ? 1.f : 0);
        if (input.moveX == 0 && input.moveY == 0) {
            input.moveX = frame.controller.x;
            input.moveY = frame.controller.y;
        }
        float n = sqrtf(input.moveX * input.moveX + input.moveY * input.moveY);
        if (n > 1) {
            input.moveX /= n;
            input.moveY /= n;
        }
        input.jump = frame.controller.jump || !!(GetAsyncKeyState(VK_SPACE) & 0x8000);
        input.attack = frame.controller.attack || !!(GetAsyncKeyState(VK_LBUTTON) & 0x8000);
        input.crouch = frame.controller.crouch || !!(GetAsyncKeyState(VK_CONTROL) & 0x8000);
        input.guard = frame.controller.guard ||
                      (actionKeyboardGuardReady && !!(GetAsyncKeyState(VK_RBUTTON) & 0x8000));
    }
    SC_Input specialInput = input;

    // L1 is the left-hand primary action: guard with a shield, cast with a
    // Flame. L2 remains guest crouch/ground pound; R2 requests shield parry.
    bool leftAttack =
        frame.focus && (frame.controller.guard || !!(GetAsyncKeyState(VK_RBUTTON) & 0x8000));
    bool parry =
        frame.focus &&
        ((frame.pad.connected && frame.pad.pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ||
         !!(GetAsyncKeyState(VK_TAB) & 0x8000));
    if (!launcherBusy() &&
        equipmentActionsInput(specialInput, leftAttack, parry, frame.focus, frame.pad.slot)) {
        acc = 1. / SC_FIXED_HZ;
        equipmentActionsTick(acc, frame.focus);
        return;
    }
    if (equipmentActionButtons.suppressAttack())
        input.attack = 0;
    // Capture short taps even on render frames without a simulation tick. Consume
    // paused time instead of accumulating a catch-up burst. The host keeps running.
    characterSession.control.input.sample(input, GetTickCount());
    characterSession.control.clock.advance(dt);
    bool stepped = false;
    Vec stepFrom =
        characterSession.instance.origin + Vec{characterSession.instance.state.position[0],
                                               characterSession.instance.state.position[1],
                                               characterSession.instance.state.position[2]} *
                                               .01f;
    if (!frame.focus) {
        characterSession.control.input.clear();
        characterSession.control.clock.clear();
    } else if (SCFeedback::pending(feedbackPause.until, GetTickCount())) {
        feedbackPause.filter(input, GetTickCount());
        characterSession.control.clock.clear();
        pumpAudio();
    } else if (characterSession.control.clock.take(worldReady())) {
        input = characterSession.control.input.take(GetTickCount());

        if (equipmentActionButtons.suppressAttack())
            input.attack = 0;
        feedbackPause.filter(input, GetTickCount());
        if (!launcherBusy()) {
            combatInput(input);
            equipmentInput(input);
        }

        // Burning-floor presentation must not be overwritten by a generic guest
        // flinch. Native passive reactions were resolved before this phase.
        adapter.api->nativeDamage(characterSession.instance.id,
                                  (environmentFrame & SC_ENV_REACTION_OWNED) ? 0
                                                                             : pendingNativeHurt);
        pendingNativeHurt = 0;
        if (!guestStep(characterSession.instance.id, &input, &characterSession.instance.state,
                       &adapter.mesh.view)) {
            logMessage("Adapter frame rejected; restoring native controls");
            releaseControl();
            return;
        }
        stepped = true;

        pumpAudio();
        if (launcherApi)
            launcherApi->query(characterSession.instance.id, &launcherState);
        launcherCameraTick();
    }
    Vec p = characterSession.instance.origin + Vec{characterSession.instance.state.position[0],
                                                   characterSession.instance.state.position[1],
                                                   characterSession.instance.state.position[2]} *
                                                   .01f;
    if (p.y < -1000 && isfinite(p.y)) {
        environmentKill(acc, "outside_world");
        return;
    }
    if (!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z)) {
        logMessage("Invalid character coordinates; native controls restored");
        releaseControl();
        return;
    }
    actionFlags(false);
    put(characterSession.native.movement + 0xc4,
        read<uint32_t>(characterSession.native.movement + 0xc4) & ~0x10u);
    if (stepped && !worldSubmit(stepFrom, p)) {
        logMessage("WORLD rejected movement request; native controls restored");
        releaseControl();
        return;
    }
}

static void updateGameplay() {
    static LARGE_INTEGER freq = {}, prev = {};
    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = prev.QuadPart ? double(now.QuadPart - prev.QuadPart) / freq.QuadPart : 0;
    prev = now;
    static double acc = 0;
    acc = std::min(acc + std::min(dt, .1), .1);

    auto frame = pollGameplayFrame();
    if (!updateCharacterLifecycle(frame, acc))
        return;
    routeGameplayActions(frame);
    refreshGameplaySettings();
    if (!prepareCharacterSimulation(frame, acc))
        return;
    simulateCharacter(frame, dt, acc);
}

static void update() {
    cutsceneObserve();
    updateGameplay();
    if (!cutsceneFrame.active)
        presentationTick();
    cutsceneTick();
    if ((!characterSession.visual.focused) ||
        SCFeedback::pending(feedbackPause.until, GetTickCount()))
        characterSession.visual.mesh.freeze();
}
