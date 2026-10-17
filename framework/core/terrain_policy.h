#pragma once
#include "../sdk/character_api.h"
#include <cmath>

namespace SCTerrain {
// Classification uses the submitted (quantized) geometry, not host addresses or
// an engine's surface IDs. Dynamic contact patches are never filtered here.
inline bool staticSurface(uint32_t capabilities, const SC_Surface &surface) {
    if (!(capabilities & SC_CAP_NATIVE_WALLS))
        return true;
    double u[3], v[3];
    for (int i = 0; i < 3; ++i) {
        u[i] = double(surface.vertices[1][i]) - surface.vertices[0][i];
        v[i] = double(surface.vertices[2][i]) - surface.vertices[0][i];
    }
    double x = u[1] * v[2] - u[2] * v[1], y = u[2] * v[0] - u[0] * v[2],
           z = u[0] * v[1] - u[1] * v[0];
    double length = std::sqrt(x * x + y * y + z * z);
    return length > 0 && std::abs(y) / length >= .01;
}
} // namespace SCTerrain
