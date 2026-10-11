#pragma once
// XInput interception is native host infrastructure. The guest reads unfiltered
// samples; native polls receive the host's current action route and stable packet
// counters. Callbacks are supplied at installation, before enabling either hook.
#include <windows.h>
#include <cstring>
#include "controller.h"
#include "../../../work/minhook/include/MinHook.h"

namespace HostController {
using Filter = void (*)(XINPUT_GAMEPAD &);
using Log = void (*)(const char *, ...);
using RumbleInstaller = void (*)(HMODULE);
static Filter filterPad = nullptr;
static Log report = nullptr;

static SCController::Reader controllerReader;
static volatile LONG controllerOwnedSlot = -1, controllerNativePolls = 0;
static thread_local bool controllerReadRaw = false;

static SCController::Sample poll(DWORD now) {
    controllerReadRaw = true;
    auto result = controllerReader.poll(now);
    controllerReadRaw = false;
    return result;
}

using ControllerGetState = DWORD(WINAPI *)(DWORD, XINPUT_STATE *);
static ControllerGetState originalControllerState = nullptr, originalControllerStateEx = nullptr;
static SRWLOCK controllerPacketLock = SRWLOCK_INIT;
static XINPUT_STATE controllerPackets[XUSER_MAX_COUNT] = {};
static bool controllerPacketSeen[XUSER_MAX_COUNT] = {};

static DWORD filterResult(DWORD result, DWORD slot, XINPUT_STATE *output) {
    if (!controllerReadRaw && result == ERROR_SUCCESS && output) {
        if (LONG(slot) == InterlockedCompareExchange(&controllerOwnedSlot, -1, -1)) {
            filterPad(output->Gamepad);
            InterlockedIncrement(&controllerNativePolls);
        }
        // Filtering and the synthetic interact press must change the packet number
        // even when the physical controller has not moved between native polls.
        if (slot < XUSER_MAX_COUNT) {
            AcquireSRWLockExclusive(&controllerPacketLock);
            auto &previous = controllerPackets[slot];
            if (!controllerPacketSeen[slot]) {
                previous = *output;
                controllerPacketSeen[slot] = true;
            } else if (memcmp(&previous.Gamepad, &output->Gamepad, sizeof(XINPUT_GAMEPAD)) != 0) {
                previous.Gamepad = output->Gamepad;
                ++previous.dwPacketNumber;
            }
            output->dwPacketNumber = previous.dwPacketNumber;
            ReleaseSRWLockExclusive(&controllerPacketLock);
        }
    }
    return result;
}

static DWORD WINAPI state(DWORD slot, XINPUT_STATE *output) {
    return filterResult(originalControllerState(slot, output), slot, output);
}

static DWORD WINAPI stateEx(DWORD slot, XINPUT_STATE *output) {
    return filterResult(originalControllerStateEx(slot, output), slot, output);
}

static void install(Filter filter, Log logger, RumbleInstaller rumble) {
    filterPad = filter;
    report = logger;
    controllerReader.poll(GetTickCount());
    void *target = controllerReader.function();
    if (!target) {
        report("XInput unavailable; keyboard remains enabled");
        return;
    }
    HMODULE module = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)target, &module);
    auto status = MH_CreateHook(target, (void *)state, (void **)&originalControllerState);
    if (status == MH_OK) {
        controllerReader.setFunction((void *)originalControllerState);
        status = MH_EnableHook(target);
    }
    report("controller input hook status=%d", status);
    void *extra = module ? (void *)GetProcAddress(module, (LPCSTR)100) : nullptr;
    if (extra && extra != target) {
        status = MH_CreateHook(extra, (void *)stateEx, (void **)&originalControllerStateEx);
        if (status == MH_OK)
            status = MH_EnableHook(extra);
        report("controller extended input hook status=%d", status);
    }
    rumble(module);
}

// Hook threads and the gameplay thread share only the interlocked ownership and
// poll counters. Packet history is protected separately by its SRW lock.
static void setOwner(int slot) {
    InterlockedExchange(&controllerOwnedSlot, slot);
}

static int owner() {
    return int(InterlockedCompareExchange(&controllerOwnedSlot, -1, -1));
}

static LONG nativePollCount() {
    return InterlockedCompareExchange(&controllerNativePolls, 0, 0);
}
} // namespace HostController
