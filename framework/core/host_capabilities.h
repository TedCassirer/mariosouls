#pragma once
#include <cstdint>

namespace SCHost {
// Host facilities, distinct from SC_CAP_* (guest-adapter abilities).
enum Capability : uint32_t {
    Motion = 1u << 0,
    Health = 1u << 1,
    Stamina = 1u << 2,
    NativeDamage = 1u << 3,
    Interactions = 1u << 4,
    DynamicSupport = 1u << 5,
    SceneDepth = 1u << 6,
    BeforeHUD = 1u << 7,
    ActorPose = 1u << 8,
    Visibility = 1u << 9,
    InputRouting = 1u << 10
};

struct Capabilities {
    uint32_t available = 0;

    bool has(uint32_t required) const {
        return (available & required) == required;
    }

    void set(Capability capability, bool verified) {
        if (verified)
            available |= capability;
        else
            available &= ~uint32_t(capability);
    }
};
enum class Operation {
    Presentation,
    Replacement,
    Movement,
    Combat,
    Interaction,
    Platform,
    WorldDraw
};

// Missing capabilities fail closed: no simulated damage, collision substitute,
// interaction emulation or drawing on top of the HUD. Native play remains available.
inline uint32_t required(Operation op) {
    switch (op) {
    case Operation::Presentation:
        return ActorPose;
    case Operation::Replacement:
        return ActorPose | Visibility;
    case Operation::Movement:
        return Motion | Health | InputRouting;
    case Operation::Combat:
        return Health | Stamina | NativeDamage;
    case Operation::Interaction:
        return Interactions;
    case Operation::Platform:
        return Motion | DynamicSupport;
    case Operation::WorldDraw:
        return SceneDepth | BeforeHUD;
    }
    return ~uint32_t(0);
}

inline bool permits(Capabilities c, Operation op) {
    return c.has(required(op));
}
} // namespace SCHost
