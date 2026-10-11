#pragma once
// PTDE 1.0.2.0: ChrPhysics+38 proxy, +3c main capsule, +40 support
// capsule; +44/+48 are actor-owned FrpgPhysShapePhantomIns. Their +1c
// shapes are the main capsule and the wider secondary contact capsule.
// Mutations run in the native motion hook, before Havok consumes movement.
static SC_Body characterBody = {1, 0, 0, 0, 0, 1, 1, 0};

struct BodyShape {
    uintptr_t shape = 0;
    float originalRadius = 0, originalPoints[8] = {}, wantedRadius = 0, wantedPoints[8] = {};
};

struct BodyBinding {
    uintptr_t actor = 0, physics = 0, movement = 0, proxy = 0, parts[2] = {};
    BodyShape shapes[3];
    float originalHeight = 0, originalRadius = 0;
    bool desired = false, applied = false;
};

static BodyBinding bodyBinding;
static SRWLOCK bodyLock = SRWLOCK_INIT;

struct BodyGuard {
    BodyGuard() {
        AcquireSRWLockExclusive(&bodyLock);
    }

    ~BodyGuard() {
        ReleaseSRWLockExclusive(&bodyLock);
    }
};

static bool bodyIdentity(const BodyBinding &b) {
    return b.actor && chain(gameSlots.player, {0, 4, 0}) == b.actor &&
           read<uintptr_t>(b.actor + 0x28) == b.movement &&
           read<uintptr_t>(b.movement + 0x1c) == b.physics &&
           read<uintptr_t>(b.physics + 0x38) == b.proxy &&
           read<uintptr_t>(b.physics + 0x3c) == b.shapes[0].shape &&
           read<uintptr_t>(b.physics + 0x40) == b.shapes[1].shape &&
           read<uintptr_t>(b.physics + 0x44) == b.parts[0] &&
           read<uintptr_t>(b.physics + 0x48) == b.parts[1] &&
           read<uintptr_t>(b.parts[0] + 0x1c) == b.shapes[0].shape &&
           read<uintptr_t>(b.parts[1] + 0x1c) == b.shapes[2].shape &&
           read<uintptr_t>(b.parts[0] + 8) == b.actor && read<uintptr_t>(b.parts[1] + 8) == b.actor;
}

static bool bodyShapeValid(uintptr_t shape) {
    return read<uintptr_t>(shape) == 0x110d754 && read<uint32_t>(shape + 0xc) == 5;
}

static bool bodyCapture() {
    if (characterBody.radiusCm == 0)
        return true;
    BodyGuard guard;
    // Release is deferred to the physics thread. A rapid reactivation must reuse
    // its saved native geometry, never capture our smaller body as the original.
    if (bodyBinding.actor == player && bodyBinding.physics == posData &&
        bodyIdentity(bodyBinding)) {
        bodyBinding.desired = true;
        return true;
    }
    BodyBinding b;
    b.actor = player;
    b.physics = posData;
    b.movement = lastMap;
    b.proxy = read<uintptr_t>(posData + 0x38);
    for (int i = 0; i < 2; i++)
        b.parts[i] = read<uintptr_t>(posData + 0x44 + i * 4);
    b.shapes[0].shape = read<uintptr_t>(posData + 0x3c);
    b.shapes[1].shape = read<uintptr_t>(posData + 0x40);
    b.shapes[2].shape = read<uintptr_t>(b.parts[1] + 0x1c);
    if (!bodyIdentity(b))
        return false;
    b.originalHeight = read<float>(posData + 0x204);
    b.originalRadius = read<float>(posData + 0x208);
    if (!isfinite(b.originalHeight) || b.originalHeight < 1 || b.originalHeight > 2 ||
        !isfinite(b.originalRadius) || b.originalRadius < .2f || b.originalRadius > .6f)
        return false;
    for (int i = 0; i < 3; i++) {
        auto &s = b.shapes[i];
        if (!bodyShapeValid(s.shape) || !get(s.shape + 0x10, s.originalRadius) ||
            !get(s.shape + 0x20, s.originalPoints))
            return false;
        if (!isfinite(s.originalRadius) || s.originalRadius < .1f || s.originalRadius > .6f)
            return false;
        for (float v : s.originalPoints)
            if (!isfinite(v) || fabsf(v) > 3)
                return false;
        if (fabsf(s.originalPoints[0]) + fabsf(s.originalPoints[2]) + fabsf(s.originalPoints[4]) +
                    fabsf(s.originalPoints[6]) >
                .001f ||
            s.originalPoints[1] < s.originalPoints[5])
            return false;
        auto cap = SCBody::capsule(i == 2 ? characterBody.hurtRadiusCm : characterBody.radiusCm,
                                   i == 2 ? characterBody.hurtHeightCm : characterBody.heightCm);
        s.wantedRadius =
            i == 1 ? cap.radius * (s.originalRadius / b.shapes[0].originalRadius) : cap.radius;
        s.wantedPoints[1] = cap.top;
        s.wantedPoints[5] = cap.bottom;
        s.wantedPoints[3] = s.wantedPoints[7] = s.wantedRadius;
    }
    b.desired = true;
    bodyBinding = b;
    logf("BODY bound original radius=%.3f height=%.3f; movement=%.3f/%.3f secondary=%.3f/%.3f",
         b.originalRadius, b.originalHeight, characterBody.radiusCm * .01f,
         characterBody.heightCm * .01f, characterBody.hurtRadiusCm * .01f,
         characterBody.hurtHeightCm * .01f);
    return true;
}

static void bodyRelease() {
    AcquireSRWLockExclusive(&bodyLock);
    bodyBinding.desired = false;
    ReleaseSRWLockExclusive(&bodyLock);
}

static void bodyNativeMotion(uintptr_t physics) {
    AcquireSRWLockExclusive(&bodyLock);
    auto &b = bodyBinding;
    if (b.physics != physics) {
        ReleaseSRWLockExclusive(&bodyLock);
        return;
    }
    if (!bodyIdentity(b)) {
        b = {};
        ReleaseSRWLockExclusive(&bodyLock);
        return;
    }
    for (const auto &s : b.shapes)
        if (!bodyShapeValid(s.shape)) {
            b = {};
            ReleaseSRWLockExclusive(&bodyLock);
            return;
        }
    for (const auto &s : b.shapes) {
        put(s.shape + 0x10, b.desired ? s.wantedRadius : s.originalRadius);
        if (b.desired)
            put(s.shape + 0x20, s.wantedPoints);
        else
            put(s.shape + 0x20, s.originalPoints);
    }
    put(b.physics + 0x204, b.desired ? characterBody.heightCm * .01f : b.originalHeight);
    put(b.physics + 0x208, b.desired ? characterBody.radiusCm * .01f : b.originalRadius);
    b.applied = b.desired;
    if (!b.desired)
        b = {};
    ReleaseSRWLockExclusive(&bodyLock);
}
