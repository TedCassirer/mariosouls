#pragma once
// Native actions receive bounded input; native locomotion never borrows the character's stick.
enum class ActionRoute { Free, Character, Locked, Ladder, Menu };
static volatile LONG actionRoute = 0, actionButton = 0, actionUntil = 0, actionPadSource = 0;
static bool actionInputReady = false;
static volatile LONG equipmentGuardInput = 0;
static volatile LONG actionBlockHeldA = 0;
// Renewed by scripted scene-camera ownership; expiry prevents stuck input.
static volatile LONG actionAimUntil = 0;

static bool actionAimOwned() {
    DWORD until = DWORD(InterlockedCompareExchange(&actionAimUntil, 0, 0));
    return until && int32_t(until - GetTickCount()) > 0;
}

static unsigned actionGuardKeys[7] = {}, actionGuardKeyCount = 0;
static bool actionKeyboardGuardReady = false;
static DWORD actionKeyboardPolls = 0;

// F737E0 resolves modifier-only bindings; 644750 maps its native key enums
// through 12C2A20 to these DirectInput scan codes. Saved game options can
// differ from DarkSouls.ini, so only the current native config is authoritative.
static constexpr unsigned actionModifierScan(unsigned index) {
    constexpr unsigned scans[] = {DIK_LCONTROL, DIK_RCONTROL, DIK_LSHIFT,
                                  DIK_RSHIFT,   DIK_LMENU,    DIK_RMENU};
    return index < 6 ? scans[index] : 0;
}

static bool actionDecodeGuardBinding(unsigned binding, unsigned (&scans)[7], unsigned &count) {
    count = 0;
    if (!binding || (binding & ~0x3f00ffu))
        return false;
    if (binding & 255) {
        unsigned scan = MapVirtualKeyA(binding & 255, MAPVK_VK_TO_VSC_EX);
        scan = (scan & 0xff) | ((scan & 0xff00) ? 0x80 : 0);
        if (!scan || scan >= 256)
            return false;
        scans[count++] = scan;
    }
    for (unsigned i = 0; i < 6; i++)
        if (binding & (0x10000u << i))
            scans[count++] = actionModifierScan(i);
    return count > 0;
}

static bool actionReadGuardBinding(unsigned &binding) {
    static int verified = 0;
    if (!verified) {
        // F72480 obtains this InputManager. F7268E accesses its keyboard config;
        // F73FF0 refreshes it from saved options (GameDataMan+34, options+1A).
        const unsigned char managerCode[] = {0xa1, 0x58, 0x84, 0x37, 0x01, 0x8b, 0x40, 0x08, 0xc3};
        const unsigned char configCode[] = {0x8b, 0x4b, 0x2c, 0x85, 0xc9, 0x0f,
                                            0x84, 0xc2, 0x01, 0x00, 0x00};
        unsigned char actualManager[sizeof(managerCode)] = {},
                      actualConfig[sizeof(configCode)] = {};
        verified = get(uintptr_t(0xf72480), actualManager) &&
                           !memcmp(actualManager, managerCode, sizeof(managerCode)) &&
                           get(uintptr_t(0xf7268e), actualConfig) &&
                           !memcmp(actualConfig, configCode, sizeof(configCode))
                       ? 1
                       : -1;
        logMessage("Equipment keyboard config signature=%d", verified == 1);
    }
    uint32_t manager = 0, config = 0, vtable = 0;
    if (verified != 1 || !get(uintptr_t(0x1378458), manager) || manager < 0x10000 ||
        !get(uintptr_t(manager) + 0x2c, config) || config < 0x10000 ||
        !get(uintptr_t(config), vtable) || vtable != 0x116bb5c ||
        !get(uintptr_t(config) + 0x50, binding))
        return false;
    return true;
}

static void actionRefreshGuardBinding() {
    unsigned binding = 0, scans[7] = {}, count = 0;
    bool valid = actionReadGuardBinding(binding) && actionDecodeGuardBinding(binding, scans, count);
    static unsigned previous = ~0u;
    static bool previouslyValid = false;
    if (binding != previous || valid != previouslyValid) {
        logMessage("Equipment native keyboard guard binding=%x ready=%d", binding, valid);
        previous = binding;
        previouslyValid = valid;
    }
    actionKeyboardGuardReady = valid;
    actionGuardKeyCount = valid ? count : 0;
    if (valid)
        memcpy(actionGuardKeys, scans, sizeof(scans));
}

static unsigned actionCombatKeys[3][7] = {}, actionCombatCounts[3] = {};

static bool actionCombatBindingReady(int pulse) {
    if (pulse < 3 || pulse > 5)
        return false;
    unsigned guard = 0, binding = 0;
    uint32_t manager = 0, config = 0;
    if (!actionReadGuardBinding(guard) || !get(uintptr_t(0x1378458), manager) ||
        !get(uintptr_t(manager) + 0x2c, config) ||
        !get(uintptr_t(config) + (pulse == 3   ? 0x54
                                  : pulse == 4 ? 0x48
                                               : 0x50),
             binding))
        return false;
    return actionDecodeGuardBinding(binding, actionCombatKeys[pulse - 3],
                                    actionCombatCounts[pulse - 3]);
}

static void actionQueue(int button, DWORD now, bool fromPad = false) {
    InterlockedExchange(&actionPadSource, fromPad);
    InterlockedExchange(&actionButton, button);
    InterlockedExchange(&actionUntil, LONG(now + 150));
}

static int actionPulse(bool forPad) {
    return bool(InterlockedCompareExchange(&actionPadSource, 0, 0)) == forPad &&
                   int32_t(DWORD(InterlockedCompareExchange(&actionUntil, 0, 0)) - GetTickCount()) >
                       0
               ? int(InterlockedCompareExchange(&actionButton, 0, 0))
               : 0;
}

static void actionCancelPulse() {
    InterlockedExchange(&actionUntil, 0);
    InterlockedExchange(&actionButton, 0);
}

static ActionRoute actionCurrentRoute() {
    return ActionRoute(InterlockedCompareExchange(&actionRoute, 0, 0));
}

static void actionFilterPad(XINPUT_GAMEPAD &p) {
    auto route = actionCurrentRoute();
    if (route == ActionRoute::Free)
        return;
    if (actionAimOwned()) {
        p.sThumbRX = p.sThumbRY = 0;
        p.wButtons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;
    }
    if (route != ActionRoute::Menu) {
        p.sThumbLX = 0;
        if (route != ActionRoute::Ladder)
            p.sThumbLY = 0;
        p.bLeftTrigger = p.bRightTrigger = 0;
        p.wButtons &= ~(XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y |
                        XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER);
    } else {
        p.wButtons &= ~XINPUT_GAMEPAD_Y;
        if (InterlockedCompareExchange(&actionBlockHeldA, 0, 0))
            p.wButtons &= ~XINPUT_GAMEPAD_A;
    }
    if (route == ActionRoute::Character && InterlockedCompareExchange(&equipmentGuardInput, 0, 0))
        p.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
    int pulse = actionPulse(true);
    if (pulse == 1)
        p.wButtons |= XINPUT_GAMEPAD_A;
    if (pulse == 2)
        p.wButtons |= XINPUT_GAMEPAD_X;
    if (pulse == 3)
        p.bLeftTrigger = 255;
    if (pulse == 4)
        p.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (pulse == 5)
        p.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
}

using ActionDeviceStateFn = HRESULT(WINAPI *)(IDirectInputDevice8W *, DWORD, LPVOID);
static ActionDeviceStateFn actionOriginalDeviceState = nullptr;
static unsigned actionKeys[12] = {};

static HRESULT WINAPI actionDeviceState(IDirectInputDevice8W *d, DWORD size, LPVOID data) {
    HRESULT result = actionOriginalDeviceState(d, size, data);
    auto route = actionCurrentRoute();
    if (FAILED(result) || !data)
        return result;

    if (route == ActionRoute::Free)
        return result;
    if (size == 256) {
        auto keys = (BYTE *)data;
        ++actionKeyboardPolls;
        if (route != ActionRoute::Menu) {
            for (int i = 0; i < 10; ++i) {
                if (route == ActionRoute::Ladder && (i == 0 || i == 1))
                    continue;
                if (actionKeys[i] < 256)
                    keys[actionKeys[i]] = 0;
            }
            // Suppress the previous binding before refreshing, including changes made
            // in the native controls menu. An invalid config never synthesizes input.
            for (unsigned i = 0; i < actionGuardKeyCount; i++)
                keys[actionGuardKeys[i]] = 0;
            actionRefreshGuardBinding();
            for (unsigned i = 0; i < actionGuardKeyCount; i++)
                keys[actionGuardKeys[i]] = 0;
            if (route == ActionRoute::Character && actionKeyboardGuardReady &&
                InterlockedCompareExchange(&equipmentGuardInput, 0, 0))
                for (unsigned i = 0; i < actionGuardKeyCount; i++)
                    keys[actionGuardKeys[i]] = 0x80;
        }
        if (route != ActionRoute::Menu)
            for (int p = 3; p <= 5; p++) {
                for (unsigned i = 0; i < actionCombatCounts[p - 3]; i++)
                    keys[actionCombatKeys[p - 3][i]] = 0;
                bool ready = actionCombatBindingReady(p);
                if (ready)
                    for (unsigned i = 0; i < actionCombatCounts[p - 3]; i++)
                        keys[actionCombatKeys[p - 3][i]] =
                            (actionPulse(false) == p ||
                             (p == 5 && route == ActionRoute::Character &&
                              InterlockedCompareExchange(&equipmentGuardInput, 0, 0)))
                                ? 0x80
                                : 0;
            }
        // Q/E cannot leak through ahead of the action eligibility check.
        if (actionKeys[10] < 256)
            keys[actionKeys[10]] = 0;
        if (route != ActionRoute::Menu && actionKeys[11] < 256)
            keys[actionKeys[11]] = 0;
        int pulse = actionPulse(false);
        if ((pulse == 1 || pulse == 2) && actionKeys[pulse == 1 ? 10 : 11] < 256)
            keys[actionKeys[pulse == 1 ? 10 : 11]] = 0x80;
    } else if ((size == sizeof(DIMOUSESTATE) || size == sizeof(DIMOUSESTATE2)) &&
               route != ActionRoute::Menu) {
        auto mouse = (DIMOUSESTATE *)data;
        mouse->rgbButtons[0] = mouse->rgbButtons[1] = 0;
        if (actionAimOwned())
            mouse->lX = mouse->lY = 0;
    }
    return result;
}

static void installActionInput() {
    char sys[MAX_PATH];
    GetSystemDirectoryA(sys, MAX_PATH);
    strcat(sys, "\\dinput8.dll");
    HMODULE dll = LoadLibraryA(sys);
    using CreateFn = HRESULT(WINAPI *)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
    auto create = (CreateFn)GetProcAddress(dll, "DirectInput8Create");
    IDirectInput8W *input = nullptr;
    IDirectInputDevice8W *keyboard = nullptr;
    if (!create || FAILED(create(GetModuleHandleA(nullptr), 0x800, IID_IDirectInput8W,
                                 (void **)&input, nullptr)))
        return;
    if (SUCCEEDED(input->CreateDevice(GUID_SysKeyboard, &keyboard, nullptr))) {
        void *target = (*(void ***)keyboard)[9];
        auto status =
            MH_CreateHook(target, (void *)actionDeviceState, (void **)&actionOriginalDeviceState);
        actionInputReady = status == MH_OK && MH_EnableHook(target) == MH_OK;
        logMessage("Action keyboard input hook=%d ready=%d", status, actionInputReady);
        keyboard->Release();
    }
    input->Release();
    const int keys[] = {'W', 'S', 'A',    'D',      VK_SPACE, VK_CONTROL,
                        'H', 'U', VK_TAB, VK_SHIFT, 'Q',      'E'};
    for (int i = 0; i < 12; i++)
        actionKeys[i] = MapVirtualKeyA(keys[i], MAPVK_VK_TO_VSC);
    // InputManager can be absent during early installation; each poll retries.
    actionRefreshGuardBinding();
}
