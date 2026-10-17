#pragma once
#include <cstdint>

// Guest-format identifiers from the pinned libsm64 src/decomp/include/sm64.h.
// Keeping the small metadata subset here avoids importing engine-wide macros
// and platform headers into the SDK. These flags never grant native immunity.
namespace MarioAction {
constexpr uint32_t ACT_TURNING_AROUND = 0x00000443;
constexpr uint32_t ACT_FINISH_TURNING_AROUND = 0x00000444;
constexpr uint32_t ACT_FLAG_AIR = 0x00000800;
constexpr uint32_t ACT_IN_CANNON = 0x00001371;
constexpr uint32_t ACT_FLAG_INVULNERABLE = 0x00020000;
constexpr uint32_t MARIO_PUNCHING = 0x00100000;
constexpr uint32_t MARIO_KICKING = 0x00200000;
constexpr uint32_t ACT_GROUND_POUND_LAND = 0x0080023c;
constexpr uint32_t ACT_PUNCHING = 0x00800380;
constexpr uint32_t ACT_MOVE_PUNCHING = 0x00800457;
constexpr uint32_t ACT_SLIDE_KICK_SLIDE = 0x0080045a;
constexpr uint32_t ACT_GROUND_POUND = 0x008008a9;
constexpr uint32_t ACT_DIVE_SLIDE = 0x00880456;
constexpr uint32_t ACT_SHOT_FROM_CANNON = 0x00880898;
constexpr uint32_t ACT_FREEFALL = 0x0100088c;
constexpr uint32_t ACT_LAVA_BOOST = 0x010208b7;
constexpr uint32_t ACT_LAVA_BOOST_LAND = 0x08000239;
constexpr uint32_t ACT_BURNING_GROUND = 0x00020449;
constexpr uint32_t ACT_BURNING_JUMP = 0x010208b4;
constexpr uint32_t ACT_BURNING_FALL = 0x010208b5;
constexpr uint32_t ACT_SLIDE_KICK = 0x018008aa;
constexpr uint32_t ACT_JUMP_KICK = 0x018008ac;
constexpr uint32_t ACT_DIVE = 0x0188088a;
constexpr uint32_t ACT_WALKING = 0x04000440;
constexpr uint32_t ACT_BRAKING = 0x04000445;
constexpr uint32_t ACT_DECELERATING = 0x0400044a;
constexpr uint32_t ACT_JUMP_LAND = 0x04000470;
constexpr uint32_t ACT_FREEFALL_LAND = 0x04000471;
constexpr uint32_t ACT_DOUBLE_JUMP_LAND = 0x04000472;
constexpr uint32_t ACT_SIDE_FLIP_LAND = 0x04000473;
constexpr uint32_t ACT_JUMP_LAND_STOP = 0x0c000230;
constexpr uint32_t ACT_DOUBLE_JUMP_LAND_STOP = 0x0c000231;
constexpr uint32_t ACT_FREEFALL_LAND_STOP = 0x0c000232;
constexpr uint32_t ACT_SIDE_FLIP_LAND_STOP = 0x0c000233;
constexpr uint32_t ACT_BRAKING_STOP = 0x0c00023d;
constexpr uint32_t ACT_IDLE = 0x0c400201;
constexpr uint32_t ACT_PANTING = 0x0c400205;
constexpr uint32_t ACT_STANDING_AGAINST_WALL = 0x0c400209;
} // namespace MarioAction
