#pragma once
#include "../../sdk/character_api.h"
#include "../../sdk/class_api.h"

// Adapter implementation fragment, after profile.generated.h.
// Immutable optional profiles and their version/size gates; no guest lifecycle work.

static const SC_UnarmedEquipmentProfile marioGloves = {
    sizeof(SC_UnarmedEquipmentProfile),
    SC_UNARMED_EQUIPMENT_PROFILE_VERSION,
    SC_UNARMED_STANDARD_FIST,
    15,
    "Mario's Gloves",
    "Equip in the right hand to use Mario attacks.",
    "White gloves made for a wandering hero.\n"
    "Equip in the active right hand for punches,\nkicks, stomps and ground pounds.\n\n"
    "Reinforce the normal path with titanite.\nEach level adds 10 base Mario power, to +15.\n"
    "Strength and Dexterity add further power.\nNative weapon ATK is not Mario move damage."};

extern "C" __declspec(dllexport) const SC_UnarmedEquipmentProfile *
SoulsCharacterGetUnarmedEquipmentProfile(uint32_t version, uint32_t size) {
    return version == SC_UNARMED_EQUIPMENT_PROFILE_VERSION && size == sizeof(marioGloves)
               ? &marioGloves
               : nullptr;
}

static const SC_ReactionProfile marioReactionProfile = {
    sizeof(SC_ReactionProfile), SC_REACTION_PROFILE_VERSION, 33, 20.f, 1.25f};
static const SC_RecoveryProfile marioRecoveryProfile = {
    sizeof(SC_RecoveryProfile), SC_RECOVERY_PROFILE_VERSION, 500, 1000, 4.f};

extern "C" __declspec(dllexport) const SC_RecoveryProfile *
SoulsCharacterGetRecoveryProfile(uint32_t version, uint32_t size) {
    return version == SC_RECOVERY_PROFILE_VERSION && size == sizeof(SC_RecoveryProfile)
               ? &marioRecoveryProfile
               : nullptr;
}

extern "C" __declspec(dllexport) const SC_ReactionProfile *
SoulsCharacterGetReactionProfile(uint32_t version, uint32_t size) {
    return version == SC_REACTION_PROFILE_VERSION && size == sizeof(SC_ReactionProfile)
               ? &marioReactionProfile
               : nullptr;
}

extern "C" __declspec(dllexport) const SC_CameraProfile *
SoulsCharacterGetCameraProfile(uint32_t version, uint32_t size) {
    return version == SC_CAMERA_PROFILE_VERSION && size == sizeof(SC_CameraProfile)
               ? &characterCameraProfile
               : nullptr;
}

// SMB-inspired low, bouncing fireball. Native host retains casting and damage.
static const SC_ProjectileProfile flowerProjectile = {sizeof(SC_ProjectileProfile),
                                                      SC_PROJECTILE_PROFILE_VERSION,
                                                      SC_PROJECTILE_FIREBALL,
                                                      3.5f,
                                                      1200,
                                                      2400,
                                                      600,
                                                      3,
                                                      75};

extern "C" __declspec(dllexport) const SC_ImpactProfile *
SoulsCharacterGetImpactProfile(uint32_t version, uint32_t size) {
    return version == SC_IMPACT_PROFILE_VERSION && size == sizeof(SC_ImpactProfile)
               ? &characterImpactProfile
               : nullptr;
}

extern "C" __declspec(dllexport) const SC_ProjectileProfile *
SoulsCharacterGetProjectileProfile(uint32_t version, uint32_t size) {
    return version == SC_PROJECTILE_PROFILE_VERSION && size == sizeof(SC_ProjectileProfile)
               ? &flowerProjectile
               : nullptr;
}

static const SC_ClassProfile startingClass = {
    sizeof(SC_ClassProfile),
    SC_CLASS_FIRECASTER,
    "Mario",
    "Hero from another world.\nEquip gloves for punches and kicks.\nFind a Fire Flower ahead.",
    160.f,
    3.14159265f};

extern "C" __declspec(dllexport) const SC_ClassProfile *
SoulsCharacterGetClassProfile(uint32_t version, uint32_t size) {
    return version == SC_CLASS_PROFILE_VERSION && size == sizeof(startingClass) ? &startingClass
                                                                                : nullptr;
}
