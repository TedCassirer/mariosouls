#pragma once
#include <cstdint>

// Internal C++ renderer boundary, not a DLL ABI. No graphics API handles cross it.
// Borrowed buffers are valid only during a submission; matrices are row-major.
// Host supplies its projection/depth conventions. A future backend must translate
// them explicitly; there is no assumption that another edition shares D3D9 state.
namespace SCRenderAPI {
struct Vertex {
    float x, y, z;
    uint32_t color, shade;
    float u, v;
};

struct Camera {
    float world[16], view[16], projection[16];
};

struct Texture {
    const uint8_t *rgba;
    uint32_t width, height;
};

struct Mesh {
    const Vertex *vertices;
    uint32_t triangles;
};
enum class Phase { Scene, BeforeHUD, Preview };

struct Context {
    Phase phase;
    bool depthAvailable;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual bool texture(Texture) = 0;
    virtual void camera(const Camera &) = 0;
    virtual bool mesh(Mesh, Context) = 0;
};

static_assert(sizeof(Vertex) == 28, "Renderer vertex layout");
} // namespace SCRenderAPI
