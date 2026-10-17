#pragma once
#include <cmath>

namespace SCPresentation {
// Row-major affine transforms, with translation in the final row.
inline bool rigid(const float *m) {
    for (int i = 0; i < 16; ++i)
        if (!std::isfinite(m[i]))
            return false;
    if (std::abs(m[3]) > .001f || std::abs(m[7]) > .001f || std::abs(m[11]) > .001f ||
        std::abs(m[15] - 1) > .001f)
        return false;
    for (int r = 0; r < 3; ++r) {
        float length = 0;
        for (int c = 0; c < 3; ++c)
            length += m[r * 4 + c] * m[r * 4 + c];
        if (std::abs(length - 1) > .02f)
            return false;
        for (int s = r + 1; s < 3; ++s) {
            float dot = 0;
            for (int c = 0; c < 3; ++c)
                dot += m[r * 4 + c] * m[s * 4 + c];
            if (std::abs(dot) > .02f)
                return false;
        }
    }
    float det = m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) +
                m[2] * (m[4] * m[9] - m[5] * m[8]);
    return std::abs(det - 1) < .03f;
}

inline void transform(const float *m, const float *p, float *out, bool point) {
    for (int c = 0; c < 3; ++c)
        out[c] = p[0] * m[c] + p[1] * m[4 + c] + p[2] * m[8 + c] + (point ? m[12 + c] : 0);
}

inline void view(const float *camera, float *out) {
    for (int i = 0; i < 16; ++i)
        out[i] = 0;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            out[r * 4 + c] = camera[c * 4 + r];
    for (int c = 0; c < 3; ++c)
        out[12 + c] = -(camera[12] * out[c] + camera[13] * out[4 + c] + camera[14] * out[8 + c]);
    out[15] = 1;
}
} // namespace SCPresentation
