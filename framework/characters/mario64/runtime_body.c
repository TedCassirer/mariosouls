/* Adapter-owned extension; compiled beside the pinned guest dependency. */
#include "libsm64.h"
#include <math.h>
float scMarioWallRadius = 50.0f;
float scMarioBodyHeight = 160.0f;

SM64_LIB_FN void sm64_host_body(float radius, float height) {
    if (!isfinite(radius) || !isfinite(height) || radius < 10 || radius > 50 ||
        height < radius * 2 || height > 160)
        return;
    scMarioWallRadius = radius;
    scMarioBodyHeight = height;
}

int scMarioOpenWorld = 0;

SM64_LIB_FN void sm64_host_open_world(int enabled) {
    scMarioOpenWorld = !!enabled;
}
