#pragma once
#include <windows.h>
#include "native_types.h"
#include "../../core/character_storage.h"
#include "../../core/mesh_interpolation.h"
#include "../../core/flag_lease.h"

namespace HostCharacter {
using MeshBuffers = SCCharacterStorage::MeshBuffers;

// Process/DLL lifetime: loader owns module and borrowed API/descriptor pointers.
// CPU buffers/atlas survive guest destruction and graphics reset. Failure in
// loader.h shuts down before FreeLibrary; there is no supported hot unload.
struct AdapterResources {
    HMODULE module = nullptr;
    const SC_CharacterAPI *api = nullptr;
    const SC_Descriptor *profile = nullptr;
    MeshBuffers mesh;
};

struct Instance {
    int id = -1;
    Vec origin{};
    SC_State state{};
};

struct VisualState {
    SCVisual::MeshHistory mesh;
    SCVisual::Anchor anchor;
    float alpha = 1;
    bool focused = false;

    void clear() {
        mesh.clear();
        anchor.clear();
    }
};

// A lease is evidence for restoration, never a second source of native state.
// releaseControl validates current actor/movement before restoring saved bits.
struct NativeLease {
    uintptr_t actor = 0, movement = 0;
    uint32_t flags1 = 0, mapFlags = 0;
    SCFlags::Lease controlLocks;
    bool hide = false, disableMapHit = false;
};

struct PresentationState {
    bool active = false;
    SCSession::Identity identity{};
    Vec previous{};
    DWORD time = 0, retry = 0;
};

// One gameplay/presentation session. Native observations, preview/cutscene and
// graphics have distinct lifetimes. Cleanup is explicitly sequenced in the
// composition root; destructors must not restore native pointers at DLL detach.
struct Session {
    Session() = default;
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    SCCharacterStorage::ControlState control;
    Instance instance;
    VisualState visual;
    NativeLease native;
    PresentationState presentation;
};
} // namespace HostCharacter
