#pragma once
// Typed bindings for the pinned, locally built guest runtime. This is private
// to Mario's adapter; the host only calls the versioned character SDK.
#include <windows.h>
#include <cstdio>
#include "../../sdk/character_api.h"
#include "../../../work/libsm64/src/libsm64.h"

struct MarioRuntimeLibrary {
    HMODULE module = nullptr;
    uint32_t (*beginCannon)(int32_t, const float *, float) = nullptr;
    void (*cancelCannon)(int32_t) = nullptr;
    uint32_t (*queryCannon)(int32_t, float *) = nullptr;
    decltype(&sm64_global_init) initialize = nullptr;
    decltype(&sm64_global_terminate) terminate = nullptr;
    decltype(&sm64_audio_init) initializeAudio = nullptr;
    decltype(&sm64_audio_tick) audioTick = nullptr;
    decltype(&sm64_static_surfaces_load) loadStaticSurfaces = nullptr;
    decltype(&sm64_host_contacts_load) loadHostContacts = nullptr;
    decltype(&sm64_mario_create) createMario = nullptr;
    decltype(&sm64_mario_delete) deleteMario = nullptr;
    decltype(&sm64_mario_tick) tickMario = nullptr;
    decltype(&sm64_mario_host_pose_tick) poseMario = nullptr;
    decltype(&sm64_mario_host_resume) resumeMario = nullptr;
    decltype(&sm64_mario_hand_matrix) handMatrix = nullptr;
    uint32_t (*setEquipment)(int32_t, uint32_t, uint32_t, uint32_t, float, float) = nullptr;
    uint32_t (*equipmentHands)(int32_t, float *, float *) = nullptr;
    decltype(&sm64_set_mario_position) setPosition = nullptr;
    decltype(&sm64_set_mario_velocity) setVelocity = nullptr;
    decltype(&sm64_set_mario_forward_velocity) setForwardVelocity = nullptr;
    decltype(&sm64_set_mario_action) setAction = nullptr;
    decltype(&sm64_set_mario_faceangle) setFacing = nullptr;
    void (*setBody)(float, float) = nullptr;
    void (*setOpenWorld)(int) = nullptr;
    void (*applyNativeHurt)(int32_t, int32_t) = nullptr;
    uint32_t (*startBurning)(int32_t, uint32_t) = nullptr;
    decltype(&sm64_surface_find_floor_height) findFloorHeight = nullptr;
    uint32_t (*probeAttack)(int32_t, float, float, float) = nullptr;
    decltype(&sm64_mario_attack) attackContact = nullptr;

    // Resolve every required symbol before initializing libsm64. The caller owns
    // rollback and unload, including a failure part-way through this sequence.
    bool bindExports(SC_Log report) {
        // Optional extension: older runtimes retain normal gameplay, but do not
        // advertise the new environment API. Update runtime + adapter together.
        startBurning = reinterpret_cast<decltype(startBurning)>(
            GetProcAddress(module, "sm64_mario_host_burning"));
        return bind(initialize, "sm64_global_init", report) &&
               bind(terminate, "sm64_global_terminate", report) &&
               bind(initializeAudio, "sm64_audio_init", report) &&
               bind(audioTick, "sm64_audio_tick", report) &&
               bind(setVelocity, "sm64_set_mario_velocity", report) &&
               bind(setForwardVelocity, "sm64_set_mario_forward_velocity", report) &&
               bind(loadStaticSurfaces, "sm64_static_surfaces_load", report) &&
               bind(loadHostContacts, "sm64_host_contacts_load", report) &&
               bind(createMario, "sm64_mario_create", report) &&
               bind(deleteMario, "sm64_mario_delete", report) &&
               bind(tickMario, "sm64_mario_tick", report) &&
               bind(poseMario, "sm64_mario_host_pose_tick", report) &&
               bind(resumeMario, "sm64_mario_host_resume", report) &&
               bind(handMatrix, "sm64_mario_hand_matrix", report) &&
               bind(setEquipment, "sm64_mario_equipment", report) &&
               bind(equipmentHands, "sm64_mario_equipment_hands", report) &&
               bind(setPosition, "sm64_set_mario_position", report) &&
               bind(setAction, "sm64_set_mario_action", report) &&
               bind(setFacing, "sm64_set_mario_faceangle", report) &&
               bind(applyNativeHurt, "sm64_mario_host_health", report) &&
               bind(findFloorHeight, "sm64_surface_find_floor_height", report) &&
               bind(probeAttack, "sm64_mario_probe_attack", report) &&
               bind(attackContact, "sm64_mario_attack", report) &&
               bind(setBody, "sm64_host_body", report) &&
               bind(setOpenWorld, "sm64_host_open_world", report) &&
               bind(beginCannon, "sm64_mario_cannon_begin", report) &&
               bind(cancelCannon, "sm64_mario_cannon_cancel", report) &&
               bind(queryCannon, "sm64_mario_cannon_query", report);
    }

private:
    template <typename Function> bool bind(Function &function, const char *symbol, SC_Log report) {
        function = reinterpret_cast<Function>(GetProcAddress(module, symbol));
        if (!function && report) {
            char message[128];
            std::snprintf(message, sizeof(message), "Missing %s", symbol);
            report(message);
        }
        return function != nullptr;
    }
};
