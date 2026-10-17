#pragma once
#include <vector>
#include <cstdint>
#include "../sdk/character_api.h"
#include "session.h"
#include "motion.h"
#include "state.h"

namespace SCCharacterStorage {
// These policies share the host's actor lifetime. Keeping them together makes
// the cleanup paths explicit; session requests survive resets of clock/input.
struct ControlState {
    SCSession::Session session;
    SCMotion::Clock clock;
    SCMotion::InputLatch input;

    // Discard queued simulation without changing requested/active/native ownership.
    void resetSimulation() {
        clock.clear();
        input.clear();
    }
};

// The host owns SDK mesh storage; adapters only fill its borrowed view. CPU
// buffers survive D3D device resets and native/guest ownership handoffs.
struct MeshBuffers {
    std::vector<float> positions, normals, colors, uvs;
    std::vector<uint8_t> atlasPixels;
    SC_Mesh view{};

    MeshBuffers() = default;
    MeshBuffers(const MeshBuffers &) = delete;
    MeshBuffers &operator=(const MeshBuffers &) = delete;

    // Called after descriptor validation, before any adapter callback borrows
    // the arrays. Refresh the view whenever storage is resized.
    void allocate(const SC_Descriptor &profile) {
        positions.resize(profile.maxTriangles * 9);
        normals.resize(profile.maxTriangles * 9);
        colors.resize(profile.maxTriangles * 9);
        uvs.resize(profile.maxTriangles * 6);
        view = {positions.data(), normals.data(),       colors.data(),
                uvs.data(),       profile.maxTriangles, 0};
        atlasPixels.resize(profile.textureWidth * profile.textureHeight * 4);
    }
};
} // namespace SCCharacterStorage
