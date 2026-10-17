#pragma once
#include <cstdint>

// Internal C++ renderer boundary, not a DLL ABI. No graphics API handles cross it.
// Vertices/matrices are borrowed during submission; matrices are row-major.
// RGBA images are immutable with stable addresses for the host session. Backends
// may cache by image identity and clear GPU resources at device reset/recreation.
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

struct LineVertex {
    float x, y, z;
    uint32_t color;
};

struct Lines {
    const LineVertex *vertices;
    uint32_t segments;
};

// Screen positions are target pixels. The backend translates them to clip space.
struct ScreenVertex {
    float x, y, z, rhw;
    uint32_t color;
    float u, v;
};

struct ScreenMesh {
    const ScreenVertex *vertices;
    uint32_t triangles;
};

struct Viewport {
    float x, y, width, height;
};
enum class ColorMode { Vertex, Modulate, TextureAlpha };
enum class Filter { Point, Linear };

struct Material {
    Texture image{};
    ColorMode color = ColorMode::Vertex;
    Filter filter = Filter::Point;
    bool shade = false;
    bool depthTest = true, depthWrite = true;
    uint8_t alphaCutoff = 0;
    bool alphaBlend = false;
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
    virtual Viewport viewport() const = 0;
    virtual void camera(const Camera &) = 0;
    virtual bool mesh(Mesh, const Material &, Context) = 0;
    virtual bool lines(Lines, bool depthTest, Context) = 0;
    virtual bool screen(ScreenMesh, const Material &, Context) = 0;
};

static_assert(sizeof(Vertex) == 28, "Renderer vertex layout");
static_assert(sizeof(LineVertex) == 16, "Renderer line layout");
static_assert(sizeof(ScreenVertex) == 28, "Renderer screen layout");
} // namespace SCRenderAPI
