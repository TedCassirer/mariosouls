#pragma once

static bool launcherVisible() {
    return launcherAvailable && launcherApi && characterOwned() && !nativeActionOwned() &&
           !characterSession.control.session.death();
}

static void launcherDrawPart(SCRenderAPI::Renderer &renderer, SCRenderAPI::Context context,
                             unsigned part, Vec at, float yaw, float pitch) {
    float viewYaw = atan2f(cameraEye().x - at.x, cameraEye().z - at.z);
    if (!launcherApi->mesh(part, float(GetTickCount() / 33), viewYaw, &launcherMesh) ||
        launcherMesh.numTrianglesUsed > launcherMesh.capacityTriangles)
        return;

    std::vector<SCRenderAPI::Vertex> vertices;
    float position[3] = {at.x, at.y, at.z};
    if (!SCGeometry::launcherMesh(launcherPositions.data(), launcherColors.data(),
                                  launcherUV.data(), launcherMesh.numTrianglesUsed, position, yaw,
                                  pitch, vertices))
        return;
    SCRenderAPI::Material style{};
    style.image = {launcherPixels.data(), launcherApi->textureWidth, launcherApi->textureHeight};
    style.color = SCRenderAPI::ColorMode::Modulate;
    style.alphaCutoff = 127;
    renderer.mesh({vertices.data(), uint32_t(vertices.size() / 3)}, style, context);
}

static void launcherPrompt(SCRenderAPI::Renderer &renderer, SCRenderAPI::Context context,
                           const char *message) {
    if (!message || !message[0] || !presentationFontCreate())
        return;

    std::vector<SCRenderAPI::ScreenVertex> v;
    auto quad = [&](float x, float y, float w, float h, uint32_t color, float u, float t, float du,
                    float dt) { SCGeometry::quad(v, x, y, w, h, color, u, t, du, dt); };
    float w = float(strlen(message)) * 9.f + 32, x = (1024 - w) / 2;
    quad(x, 585, w, 38, 0xff17191d, 0, 0, 0, 0);
    SCGeometry::fit(v, renderer.viewport());
    renderer.screen({v.data(), 2}, {}, context);
    v.clear();
    for (const char *s = message; *s; s++, x += 9) {
        unsigned c = (unsigned char)*s;
        if (c < 32 || c > 126)
            c = '?';
        quad(x + 16, 594, 20, 20, 0xfffff5df, (c % 16) / 16.f, (c / 16) / 8.f, 1 / 16.f, 1 / 8.f);
    }
    SCGeometry::fit(v, renderer.viewport());
    SCRenderAPI::Material style{};
    style.image = presentationFont();
    style.color = SCRenderAPI::ColorMode::Modulate;
    style.alphaCutoff = 127;
    renderer.screen({v.data(), uint32_t(v.size() / 3)}, style, context);
}

static void launcherReticle(SCRenderAPI::Renderer &renderer, SCRenderAPI::Context context) {
    if (launcherState.phase != SC_LAUNCHER_AIMING)
        return;

    using V = SCRenderAPI::ScreenVertex;
    std::vector<V> v;
    for (int i = 0; i < 64; i++) {
        float a = i * 6.2831853f / 64, b = (i + 1) * 6.2831853f / 64;
        V p = {512 + cosf(a) * 240, 360 + sinf(a) * 300, 0, 1, 0xff000000},
          q = {512 + cosf(b) * 240, 360 + sinf(b) * 300, 0, 1, 0xff000000};
        V r = {512 + cosf(a) * 1600, 360 + sinf(a) * 1600, 0, 1, 0xff000000},
          s = {512 + cosf(b) * 1600, 360 + sinf(b) * 1600, 0, 1, 0xff000000};
        v.insert(v.end(), {p, r, q, q, r, s});
    }
    for (int i = 0; i < 4; i++) {
        float a = i * 1.5707963f, c = cosf(a), s = sinf(a);
        for (auto p : {Vec{23, 0, 0}, Vec{43, -13, 0}, Vec{43, 13, 0}})
            v.push_back(
                {512 + (p.x * c - p.y * s) * .8f, 360 + p.x * s + p.y * c, 0, 1, 0xb4323232});
    }
    SCGeometry::fit(v, renderer.viewport());
    renderer.screen({v.data(), uint32_t(v.size() / 3)}, {}, context);
}

static void launcherDraw(SCRenderAPI::Renderer &renderer, SCRenderAPI::Context context) {
    if (!launcherVisible())
        return;
    Vec playerAt = read<Vec>(playerPhysics + 0x10);
    if (dot(playerAt - launcherPlace, playerAt - launcherPlace) > 10000)
        return;
    launcherDrawPart(renderer, context, SC_LAUNCHER_OPERATOR, launcherBuddy, 0, 0);
    if (!launcherUnlocked)
        launcherDrawPart(renderer, context, SC_LAUNCHER_LID, launcherPlace + Vec{0, .03f, 0}, 0, 0);
    else if (launcherState.phase != SC_LAUNCHER_AIMING) {
        Vec at = launcherPlace + Vec{0, -3.35f, 0};
        if (launcherState.phase == SC_LAUNCHER_OPENING)
            at.y = characterSession.instance.origin.y + launcherState.propLift * .01f;
        float yaw = launcherState.phase == SC_LAUNCHER_OPENING ? launcherState.yaw : launcherYaw;
        float pitch = launcherState.phase == SC_LAUNCHER_OPENING ? -launcherState.pitch : 0;
        launcherDrawPart(renderer, context, SC_LAUNCHER_BASE, at, yaw, 0);
        launcherDrawPart(renderer, context, SC_LAUNCHER_BARREL, at, yaw, pitch);
    }
    char prompt[160] = {};
    const char *message = nullptr;
    if (launcherState.phase == SC_LAUNCHER_OPENING)
        message = "Preparing cannon...";
    else if (launcherState.phase == SC_LAUNCHER_AIMING)
        message = "Either stick / WASD: aim    A / Space: fire    Y / Q: leave";
    else if (launcherMessageUntil && int32_t(launcherMessageUntil - GetTickCount()) > 0)
        message = "Ready for blastoff! The cannon is open.";
    else if (launcherTarget == 1) {
        snprintf(prompt, sizeof(prompt), "A / Q: Talk to %s", launcherApi->operatorName);
        message = prompt;
    } else if (launcherTarget == 2)
        message = "A / Q: Enter cannon";
    launcherReticle(renderer, context);
    launcherPrompt(renderer, context, message);
}
