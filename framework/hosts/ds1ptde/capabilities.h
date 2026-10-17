#pragma once

// Translate PTDE's verified hooks AND current readable observations in one place.
// Feature-local flags remain evidence/diagnostics; they are not support claims.
static SCHost::Capabilities hostCapabilities(bool beforeHUD) {
    SCHost::Capabilities c{};
    int hp = -1, maximum = 0, stamina = -1;
    bool health = player && get(player + 0x2d4, hp) && get(player + 0x2d8, maximum) &&
                  maximum > 0 && hp >= 0 && hp <= maximum;
    Vec position{};
    float yaw = 0, fov = 0;
    bool pose = player && playerPhysics && playerCamera && get(playerPhysics + 0x10, position) &&
                isfinite(dot(position, position)) && get(playerPhysics + 4, yaw) && isfinite(yaw) &&
                get(playerCamera + 0x50, fov) && isfinite(fov);
    c.set(SCHost::ActorPose, pose);
    c.set(SCHost::Visibility, characterVisibilityReady);
    c.set(SCHost::Motion, worldMotionReady);
    c.set(SCHost::Health, health);
    // The PTDE damage transaction also installs the native stamina filter.
    c.set(SCHost::Stamina, characterDamageReady && player && get(player + 0x2e4, stamina) &&
                               stamina >= 0 && stamina <= 100000);
    c.set(SCHost::NativeDamage, nativeDamage && characterDamageReady);
    c.set(SCHost::Interactions, actionInputReady);
    c.set(SCHost::InputRouting, actionInputReady);
    c.set(SCHost::DynamicSupport, worldSupportReady);
    c.set(SCHost::SceneDepth, sceneDepth != nullptr);
    c.set(SCHost::BeforeHUD, beforeHUD);
    return c;
}

static bool hostPermits(SCHost::Operation operation) {
    return SCHost::permits(hostCapabilities(), operation);
}
