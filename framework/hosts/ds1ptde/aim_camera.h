#pragma once
// PTDE exports camera-to-world matrices through 404EA0 (ECX source, EAX output).
// The gameplay camera has no render handle at +4. CamSfx::submit misses its
// world-render copies. Replace only its exported matrix, never its simulation.
static SRWLOCK aimCameraLock = SRWLOCK_INIT;

struct AimCameraFrame {
    uintptr_t camera = 0, actor = 0;
    Vec eye{}, forward{};
    DWORD time = 0;
    bool active = false;
};

static AimCameraFrame aimCameraRequest, aimCameraRendered;
static bool aimCameraReady = false;
extern "C" {
__attribute__((used)) void *aimCameraOriginalCopy = nullptr;
}

static void aimCameraSet(bool active, Vec eye = {}, Vec forward = {}) {
    DWORD now = GetTickCount();
    InterlockedExchange(&actionAimUntil, active && aimCameraReady ? LONG(now + 150) : 0);
    AcquireSRWLockExclusive(&aimCameraLock);
    aimCameraRequest = {playerCamera, player, eye, forward, now, active && aimCameraReady};
    if (!active)
        aimCameraRendered = {};
    ReleaseSRWLockExclusive(&aimCameraLock);
}

extern "C" __attribute__((used, noinline)) void aimCameraCopied(uintptr_t source,
                                                                uintptr_t output) {
    AimCameraFrame frame;
    AcquireSRWLockShared(&aimCameraLock);
    frame = aimCameraRequest;
    ReleaseSRWLockShared(&aimCameraLock);
    if (!frame.active || source != frame.camera || DWORD(GetTickCount() - frame.time) >= 150 ||
        source != chain(gameSlots.camera, {0, 0x3c, 0x60}) ||
        frame.actor != chain(gameSlots.player, {0, 4, 0}) || read<int>(frame.actor + 0x2d4) <= 0)
        return;
    Vec z = unit(frame.forward), x = unit(cross({0, 1, 0}, z)), y = cross(z, x);
    if (!isfinite(dot(frame.eye, frame.eye)) || dot(z, z) < .99f || dot(x, x) < .99f)
        return;
    float transform[16] = {x.x, x.y, x.z, 0, y.x,         y.y,         y.z,         0,
                           z.x, z.y, z.z, 0, frame.eye.x, frame.eye.y, frame.eye.z, 1};
    if (!put(output, transform))
        return;
    frame.time = GetTickCount();
    frame.forward = z;
    AcquireSRWLockExclusive(&aimCameraLock);
    aimCameraRendered = frame;
    ReleaseSRWLockExclusive(&aimCameraLock);
}

// Preserve original register/SIMD results and flags. Its ECX result is source+40.
__attribute__((naked)) static void aimCameraCopyHook() {
    __asm__ volatile(
        "push %ecx\n call *_aimCameraOriginalCopy\n pushfl\n pushal\n subl $128,%esp\n"
        "movups %xmm0,0(%esp)\n movups %xmm1,16(%esp)\n movups %xmm2,32(%esp)\n movups %xmm3,48(%esp)\n"
        "movups %xmm4,64(%esp)\n movups %xmm5,80(%esp)\n movups %xmm6,96(%esp)\n movups %xmm7,112(%esp)\n"
        "movl 156(%esp),%eax\n movl 164(%esp),%ecx\n push %eax\n push %ecx\n call _aimCameraCopied\n addl $8,%esp\n"
        "movups 0(%esp),%xmm0\n movups 16(%esp),%xmm1\n movups 32(%esp),%xmm2\n movups 48(%esp),%xmm3\n"
        "movups 64(%esp),%xmm4\n movups 80(%esp),%xmm5\n movups 96(%esp),%xmm6\n movups 112(%esp),%xmm7\n"
        "addl $128,%esp\n popal\n popfl\n leal 4(%esp),%esp\n ret\n");
}

static bool aimCameraInstall() {
    if (aimCameraReady)
        return true;
    const unsigned char expected[] = {0xf3, 0x0f, 0x7e, 0x41, 0x10, 0x66, 0x0f, 0xd6, 0x00};
    unsigned char bytes[sizeof(expected)] = {};
    if (!get(uintptr_t(0x404ea0), bytes) || memcmp(bytes, expected, sizeof(bytes))) {
        logMessage("CAMERA matrix export signature refused");
        return false;
    }
    auto status =
        MH_CreateHook((void *)0x404ea0, (void *)aimCameraCopyHook, &aimCameraOriginalCopy);
    if (status == MH_OK)
        status = MH_EnableHook((void *)0x404ea0);
    aimCameraReady = status == MH_OK;
    logMessage("CAMERA scripted matrix export hook=%d", aimCameraReady);
    return aimCameraReady;
}

static Vec cameraFrameVector(bool forward) {
    AcquireSRWLockShared(&aimCameraLock);
    auto frame = aimCameraRendered;
    bool active = aimCameraRequest.active;
    ReleaseSRWLockShared(&aimCameraLock);
    if (active && frame.active && frame.camera == playerCamera && frame.actor == player &&
        DWORD(GetTickCount() - frame.time) < 150)
        return forward ? frame.forward : frame.eye;
    return read<Vec>(playerCamera + (forward ? 0x30 : 0x40));
}
