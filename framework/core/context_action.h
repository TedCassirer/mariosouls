#pragma once
#include <cstdint>

namespace SCContext {
struct Result {
    bool jump = false, interact = false, blockNative = false;
};

// Decide once per physical press. Never turn a held jump into an interaction,
// or an interaction/menu confirmation into a jump after control returns.
class Button {
    enum class Owner { None, Jump, Interaction, Suppressed, Menu };
    Owner owner = Owner::None;
    bool ready = false, wasDown = false;
    uintptr_t actor = 0;
    int slot = -1;

public:
    Result update(bool down, bool focused, uintptr_t identity, int controller, bool guestControl,
                  bool canInteract) {
        if (!focused || !identity || controller < 0 || identity != actor || controller != slot) {
            actor = identity;
            slot = controller;
            ready = false;
            owner = Owner::Suppressed;
            wasDown = down;
            if (focused && identity && controller >= 0 && !down) {
                ready = true;
                owner = Owner::None;
            }
            return {false, false, down};
        }
        if (!down) {
            ready = true;
            wasDown = false;
            owner = Owner::None;
            return {};
        }
        bool edge = ready && !wasDown;
        wasDown = true;
        if (edge)
            owner = !guestControl ? Owner::Menu : canInteract ? Owner::Interaction : Owner::Jump;
        if (!guestControl && owner == Owner::Jump)
            owner = Owner::Suppressed;
        return {guestControl && owner == Owner::Jump, edge && owner == Owner::Interaction,
                owner != Owner::Menu && owner != Owner::None};
    }
};
} // namespace SCContext
