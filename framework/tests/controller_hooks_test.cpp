#undef NDEBUG
#include "../hosts/ds1ptde/controller_hooks.h"
#include <cassert>
#include <cstdio>

// Exercise the actual native-poll wrappers with a synthetic XInput provider.
// Do not install hooks, load a game, or poll a physical controller.
namespace {
WORD routedButtons = XINPUT_GAMEPAD_A;
unsigned filtered = 0;

void route(XINPUT_GAMEPAD &pad) {
    ++filtered;
    pad.wButtons = routedButtons;
    pad.sThumbLX = pad.sThumbLY = 0;
}

DWORD WINAPI provider(DWORD slot, XINPUT_STATE *output) {
    if (slot != 1)
        return ERROR_DEVICE_NOT_CONNECTED;
    *output = {};
    output->dwPacketNumber = 40;
    output->Gamepad.wButtons = XINPUT_GAMEPAD_B;
    output->Gamepad.sThumbLX = 20000;
    output->Gamepad.sThumbRX = 12345;
    return ERROR_SUCCESS;
}
} // namespace

int main() {
    HostController::filterPad = route;
    HostController::originalControllerState = provider;
    HostController::originalControllerStateEx = provider;
    HostController::setOwner(1);
    assert(HostController::owner() == 1);

    XINPUT_STATE output{};
    assert(HostController::state(1, &output) == ERROR_SUCCESS);
    assert(output.Gamepad.wButtons == XINPUT_GAMEPAD_A);
    assert(output.Gamepad.sThumbLX == 0 && output.Gamepad.sThumbRX == 12345);
    assert(output.dwPacketNumber == 40 && filtered == 1);
    assert(HostController::nativePollCount() == 1);

    // Both XInput entry points share packet history. Repeating the same filtered
    // sample preserves its number; synthetic input changes advance it even when
    // the physical provider's packet is unchanged.
    assert(HostController::stateEx(1, &output) == ERROR_SUCCESS);
    assert(output.dwPacketNumber == 40);
    routedButtons = 0;
    HostController::stateEx(1, &output);
    assert(output.dwPacketNumber == 41 && output.Gamepad.wButtons == 0);
    routedButtons = XINPUT_GAMEPAD_A;
    HostController::state(1, &output);
    assert(output.dwPacketNumber == 42);

    // Guest polling must bypass the native route and its synthetic history.
    auto polls = HostController::nativePollCount();
    auto calls = filtered;
    HostController::controllerReadRaw = true;
    HostController::state(1, &output);
    HostController::controllerReadRaw = false;
    assert(output.dwPacketNumber == 40 && output.Gamepad.wButtons == XINPUT_GAMEPAD_B);
    assert(output.Gamepad.sThumbLX == 20000);
    assert(HostController::nativePollCount() == polls && filtered == calls);

    HostController::setOwner(-1);
    HostController::state(1, &output);
    assert(output.dwPacketNumber == 43 && output.Gamepad.wButtons == XINPUT_GAMEPAD_B);
    assert(output.Gamepad.sThumbLX == 20000 && filtered == calls);
    HostController::state(1, &output);
    assert(output.dwPacketNumber == 43);

    // A failed native call, absent output, or out-of-range unowned slot must
    // pass through without recording a packet or spending a filtered poll.
    XINPUT_STATE before = output;
    assert(HostController::state(0, &output) == ERROR_DEVICE_NOT_CONNECTED);
    assert(std::memcmp(&before, &output, sizeof(output)) == 0);
    assert(HostController::filterResult(ERROR_SUCCESS, 1, nullptr) == ERROR_SUCCESS);
    assert(HostController::filterResult(ERROR_SUCCESS, XUSER_MAX_COUNT, &output) == ERROR_SUCCESS);
    assert(std::memcmp(&before, &output, sizeof(output)) == 0);
    assert(HostController::nativePollCount() == polls && filtered == calls);

    // The native DWORD packet number wraps naturally when routed state changes.
    HostController::controllerPackets[1].dwPacketNumber = 0xffffffff;
    HostController::setOwner(1);
    HostController::state(1, &output);
    assert(output.dwPacketNumber == 0 && output.Gamepad.wButtons == XINPUT_GAMEPAD_A);
    puts("PASSED: controller hook routing and synthetic packet history");
}
