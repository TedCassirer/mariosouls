#pragma once
#include "../../sdk/character_api.h"

namespace MarioPose {
struct Clip {
    int animation, style;
    float phase;
};

// Guest animation IDs and runtime sampling styles belong here, never in the host.
inline Clip select(const SC_Pose &pose) {
    switch (pose.kind) {
    case SC_POSE_CLIMB:
        return {5, 2, pose.travelCm * .9f};
    case SC_POSE_DRINK:
        return {0x30, 4, pose.phase};
    case SC_POSE_REST:
        return {0x84, 1, pose.phase};
    case SC_POSE_DOOR:
        return {0x60, 1, pose.phase};
    case SC_POSE_WALK:
        return {0x48, 0, -1};
    case SC_POSE_DOWNED:
        return {0x19, 0, -1};
    case SC_POSE_STAGGER:
        return pose.phase >= 0 ? Clip{0x74, 1, pose.phase} : Clip{0x74, 0, -1};
    case SC_POSE_KNOCKDOWN:
        // SM64 backward ground knockback: fall onto seat and recover.
        return {0x7b, 1, pose.phase};
    case SC_POSE_DEATH:
        return {0x32, 8, -1};
    default:
        return {0xc3, 0, -1};
    }
}
} // namespace MarioPose
