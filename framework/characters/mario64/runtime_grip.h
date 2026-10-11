/* Adapter terrain policy. Ordinary host stone/earth needs traction without
 * changing flat movement, airborne velocity or deliberately slippery surfaces.
 * Keep slope acceleration, forced slides and uphill steep checks in agreement.
 */
#ifndef SC_MARIO_GRIP_H
#define SC_MARIO_GRIP_H
static int sc_mario_grip_floor(struct MarioState *m) {
    return m->floor && m->floor->type == SURFACE_DEFAULT
        && m->curTerrain != TERRAIN_SLIDE
        && m->floor->normal.y > 0.70710678f; /* cos(45 degrees) */
}
#endif
