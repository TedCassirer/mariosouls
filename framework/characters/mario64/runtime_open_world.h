/* SM64 treats a NULL floor as the boundary of its world, stopping horizontal
 * motion and sometimes reusing the last floor height as an invisible landing.
 * In host motion mode, absence of guest geometry means freefall instead.
 * This sentinel only supplies the non-null floor data required by Mario's
 * action code. It is never inserted into collision geometry; public height
 * probes still return the ordinary missing-floor limit. Host origin rebasing
 * keeps gameplay far above this limit, and native kill surfaces remain lethal.
 */
extern int scMarioOpenWorld;
static struct SM64SurfaceCollisionData *sc_mario_open_floor(void) {
    static struct SM64SurfaceCollisionData floor={
        .type=0, .lowerY=-11000, .upperY=-11000,
        .vertex1={-32767,-11000,-32767},
        .vertex2={-32767,-11000,32767},
        .vertex3={32767,-11000,32767},
        .normal={0,1,0}, .originOffset=11000, .isValid=1
    };
    return &floor;
}
