#pragma once
#include "native_fire.h"
// E7FE14 invokes E6BFE0 with EAX=target, stack=attacker/packet/preview,
// callee pops 12. Observe around the original; preserve all registers, flags,
// x87/SSE/MXCSR around callbacks. Damage and native return values are unchanged.
extern "C" {
__attribute__((used)) void *environmentOriginalApply = nullptr;
}
static bool environmentFireReady = false;
static thread_local PTDEFire::Hit environmentFireStack[8];
static thread_local unsigned environmentFireDepth = 0;
static SRWLOCK environmentFireLock = SRWLOCK_INIT;
static PTDEFire::Hit environmentFirePending;
static DWORD environmentFireTime = 0;
static int environmentFireMap = -1;

extern "C" __attribute__((used, noinline)) void
environmentFireBefore(uintptr_t actor, uintptr_t attacker, uintptr_t packet, unsigned preview) {
    const unsigned index = environmentFireDepth++;
    if (index < 8)
        environmentFireStack[index] =
            characterOwned() && actor == player && actor == characterSession.native.actor
                ? PTDEFire::before(actor, attacker, packet, preview)
                : PTDEFire::Hit{};
}

extern "C" __attribute__((used, noinline)) void environmentFireAfter() {
    if (!environmentFireDepth)
        return;
    const unsigned index = --environmentFireDepth;
    if (index >= 8)
        return;
    auto hit = environmentFireStack[index];
    if (!PTDEFire::after(hit))
        return;

    AcquireSRWLockExclusive(&environmentFireLock);
    environmentFirePending = hit;
    environmentFireTime = GetTickCount();
    environmentFireMap = currentMap();
    ReleaseSRWLockExclusive(&environmentFireLock);
}

__attribute__((naked, used)) static void environmentFireHook() {
    __asm__ volatile(
        "pushfl\n pushal\n movl %esp,%edi\n subl $528,%esp\n andl $-16,%esp\n fxsave (%esp)\n"
        "pushl 48(%edi)\n pushl 44(%edi)\n pushl 40(%edi)\n pushl 28(%edi)\n call _environmentFireBefore\n addl $16,%esp\n"
        "fxrstor (%esp)\n movl %edi,%esp\n popal\n popfl\n"
        "pushl 12(%esp)\n pushl 12(%esp)\n pushl 12(%esp)\n call *_environmentOriginalApply\n"
        "pushfl\n pushal\n movl %esp,%edi\n subl $528,%esp\n andl $-16,%esp\n fxsave (%esp)\n"
        "call _environmentFireAfter\n fxrstor (%esp)\n movl %edi,%esp\n popal\n popfl\n ret $12\n");
}

static void environmentFireInstall() {
    if (!environmentAPI || !PTDEFire::available())
        return;
    if (MH_CreateHook((void *)0xe6bfe0, (void *)environmentFireHook, &environmentOriginalApply) ==
        MH_OK)
        environmentFireReady = MH_EnableHook((void *)0xe6bfe0) == MH_OK;
    logMessage("Native fire-bearing HP-loss observer=%d", environmentFireReady);
}

static bool environmentConsumeFire() {
    AcquireSRWLockExclusive(&environmentFireLock);
    const auto hit = environmentFirePending;
    const DWORD tick = environmentFireTime;
    const int map = environmentFireMap;
    environmentFirePending = {};
    ReleaseSRWLockExclusive(&environmentFireLock);
    return hit.candidate && player == hit.actor && player == characterSession.native.actor &&
           SCNativeMemory::read<uint32_t>(player + 0x28) == hit.movement && currentMap() == map &&
           DWORD(GetTickCount() - tick) <= 5000 && SCNativeMemory::read<int>(player + 0x2d4) > 0;
}

static void environmentReset() {
    AcquireSRWLockExclusive(&environmentFireLock);
    environmentFirePending = {};
    ReleaseSRWLockExclusive(&environmentFireLock);
}
