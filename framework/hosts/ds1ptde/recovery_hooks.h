#pragma once
#include "recovery_native.h"

static bool nativeRecoveryReady = false;
static SRWLOCK nativeRecoveryLock = SRWLOCK_INIT;
static PTDERecovery::Request nativeRecoveryRequest;
extern "C" {
__attribute__((used)) void *nativeRecoveryOriginalEnd = nullptr;
}

static void nativeRecoveryReset() {
    AcquireSRWLockExclusive(&nativeRecoveryLock);
    nativeRecoveryRequest = {};
    ReleaseSRWLockExclusive(&nativeRecoveryLock);
}

extern "C" __attribute__((used, noinline)) uint32_t nativeRecoveryEnd(uintptr_t action,
                                                                      uint32_t original) {
    AcquireSRWLockShared(&nativeRecoveryLock);
    const auto request = nativeRecoveryRequest;
    ReleaseSRWLockShared(&nativeRecoveryLock);
    if (!request.actor || !characterOwned() || nativeActionOwned())
        return original;
    const uintptr_t actor = chain(gameSlots.player, {0, 4, 0});
    PTDERecovery::State current;
    if (!PTDERecovery::inspect(actor, current))
        return original;
    const auto interaction = nativeInteractionState(false);
    DWORD foreground = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
    const uintptr_t tracks = read<uint32_t>(current.movement + 0x18);
    const bool paired = !tracks || (read<uint32_t>(tracks + 28 * 0x9c + 4) != 0xffffffffu &&
                                    read<int>(tracks + 28 * 0x9c) >= 0);
    const bool permitted = actor == player && actor == characterSession.native.actor &&
                           current.movement == characterSession.native.movement &&
                           interaction.valid && !interaction.menu && !interaction.event &&
                           !interaction.ladder && !paired && foreground == GetCurrentProcessId();
    if (!PTDERecovery::matches(request, actor, action, read<int>(actor + 0x2d4), currentMap(),
                               GetTickCount(), current, permitted))
        return original;
    // The native ESD still evaluates death/new-hit/throw branches first, then
    // runs its normal exit commands and state0 entry. Never write activeAllowed.
    return (original & ~255u) | 1u;
}

__attribute__((naked, used)) static void nativeRecoveryEndHook() {
    __asm__ volatile(
        "pushl %eax\n call *_nativeRecoveryOriginalEnd\n"
        "pushfl\n pushal\n movl %esp,%edi\n subl $528,%esp\n andl $-16,%esp\n fxsave (%esp)\n"
        "pushl 28(%edi)\n pushl 36(%edi)\n call _nativeRecoveryEnd\n addl $8,%esp\n movl %eax,28(%edi)\n"
        "fxrstor (%esp)\n movl %edi,%esp\n popal\n popfl\n addl $4,%esp\n ret\n");
}

static void nativeRecoveryInstall() {
    if (!defenseRecoveryReady || !PTDERecovery::available())
        return;
    if (MH_CreateHook((void *)0xe2c340, (void *)nativeRecoveryEndHook,
                      &nativeRecoveryOriginalEnd) == MH_OK)
        nativeRecoveryReady = MH_EnableHook((void *)0xe2c340) == MH_OK;
    logMessage("Bounded native ordinary-hit recovery=%d", nativeRecoveryReady);
}
