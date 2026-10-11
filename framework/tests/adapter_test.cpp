#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../sdk/character_api.h"
#include "../core/validation.h"
#include "../core/camera.h"
#include "../core/hazard.h"
#include "../core/projectile.h"
#include "../core/impact.h"
#include "../sdk/launcher_api.h"
#include "../core/starting_class.h"

static void log(const char *s) {
    printf("adapter: %s\n", s);
}

int main(int argc, char **argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 2)
        return 2;
    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) {
        printf("LoadLibrary failed %lu\n", GetLastError());
        return 3;
    }
    auto entry = (SC_GetAPI)GetProcAddress(dll, "SoulsCharacterGetAPI");
    assert(entry);
    assert(!entry(99, sizeof(SC_CharacterAPI)));
    assert(!entry(1, sizeof(SC_CharacterAPI)));
    assert(!entry(SC_ABI_VERSION, 0));
    const auto *a = entry(SC_ABI_VERSION, sizeof(SC_CharacterAPI));
    assert(SC::api(a));
    const auto *d = a->describe();
    assert(SC::descriptor(d));
    if (auto camera = (SC_GetCameraProfile)GetProcAddress(dll, "SoulsCharacterGetCameraProfile")) {
        assert(!camera(99, sizeof(SC_CameraProfile)));
        assert(!camera(SC_CAMERA_PROFILE_VERSION, 0));
        assert(SCCamera::valid(camera(SC_CAMERA_PROFILE_VERSION, sizeof(SC_CameraProfile))));
    }
    if (auto starting = (SC_GetClassProfile)GetProcAddress(dll, "SoulsCharacterGetClassProfile")) {
        assert(!starting(99, sizeof(SC_ClassProfile)));
        assert(!starting(SC_CLASS_PROFILE_VERSION, 0));
        assert(SCClass::valid(starting(SC_CLASS_PROFILE_VERSION, sizeof(SC_ClassProfile))));
    }
    std::vector<uint8_t> atlas(d->textureWidth * d->textureHeight * 4);
    if (auto impact = (SC_GetImpactProfile)GetProcAddress(dll, "SoulsCharacterGetImpactProfile")) {
        assert(!impact(99, sizeof(SC_ImpactProfile)));
        assert(!impact(SC_IMPACT_PROFILE_VERSION, 0));
        assert(SCImpact::valid(impact(SC_IMPACT_PROFILE_VERSION, sizeof(SC_ImpactProfile)),
                               d->attackCount));
    }
    if (auto projectile =
            (SC_GetProjectileProfile)GetProcAddress(dll, "SoulsCharacterGetProjectileProfile")) {
        assert(!projectile(99, sizeof(SC_ProjectileProfile)));
        assert(!projectile(SC_PROJECTILE_PROFILE_VERSION, 0));
        assert(SCProjectile::valid(
            projectile(SC_PROJECTILE_PROFILE_VERSION, sizeof(SC_ProjectileProfile))));
    }
    SC_Init init = {sizeof(SC_Init), argc > 2 ? argv[2] : "", argc > 3 ? argv[3] : "", log};
    SC_Texture tex = {atlas.data(), uint32_t(atlas.size())};
    assert(a->initialize(&init, &tex));
    assert(!a->initialize(&init, &tex));
    SC_Surface floor[2] = {{{{-16000, 0, -16000}, {-16000, 0, 16000}, {16000, 0, 16000}}, 0},
                           {{{-16000, 0, -16000}, {16000, 0, 16000}, {16000, 0, -16000}}, 0}};
    a->surfaces(floor, 2, 0);
    assert(fabsf(a->floorHeight(0, 100, 0)) < .01f);
    // Repeated creation previews must leave the gameplay instance available.
    if (GetProcAddress(dll, "SoulsCharacterGetClassProfile")) {
        std::vector<float> pp(d->maxTriangles * 9), pn(pp.size()), pc(pp.size()),
            pu(d->maxTriangles * 6);
        SC_Mesh pm = {pp.data(), pn.data(), pc.data(), pu.data(), d->maxTriangles, 0};
        SC_State ps{};
        for (int cycle = 0; cycle < 3; cycle++) {
            int preview = a->create(0, 0, 0, 0);
            assert(preview >= 0);
            for (int frame = 0; frame < 90; frame++) {
                SC_Pose pose = {};
                pose.phase = -1;
                pose.yaw = frame * .03f;
                assert(a->pose(preview, &pose, &ps, &pm));
                assert(SC::frame(ps, pm, *d));
                assert(pm.numTrianglesUsed > 0);
            }
            a->destroy(preview);
        }
        if (!strcmp(d->id, "mario64")) {
            a->surfaces(nullptr, 0, 0);
            a->surfaces(nullptr, 0, 1);
            int actor = a->create(0, 0, 0, 0);
            assert(actor >= 0);
            for (int phase : {SC_POSE_NEUTRAL, SC_POSE_DEATH, SC_POSE_REST, SC_POSE_WALK}) {
                for (int tick = 0; tick < 40; tick++) {
                    SC_Pose pose = {};
                    pose.kind = phase;
                    pose.phase = -1;
                    pose.position[0] = tick * 10.f;
                    assert(a->pose(actor, &pose, &ps, &pm));
                    assert(SC::frame(ps, pm, *d));
                    assert(pm.numTrianglesUsed > 0);
                }
            }
            a->destroy(actor);
            a->surfaces(floor, 2, 0);
            puts("PASS real appearance without cached floor: load/death/rest/walk and teardown");
        }
        puts(
            "PASS class preview: real animated mesh, rotation, repeated teardown then gameplay create");
    }
    int id = a->create(0, 30, 0, 0);
    assert(id >= 0);
    assert(a->create(0, 30, 0, 0) < 0);
    std::vector<float> pos(d->maxTriangles * 9), normal(pos.size()), color(pos.size()),
        uv(d->maxTriangles * 6);
    SC_Mesh m = {pos.data(), normal.data(), color.data(), uv.data(), d->maxTriangles, 0};
    SC_State state{};
    SC_Input input = {0, 1, 0, 0, 0, 0, 0, 0};
    auto step = [&]() {
        assert(a->step(id, &input, &state, &m));
        assert(SC::frame(state, m, *d));
        assert(m.numTrianglesUsed || (!strcmp(d->id, "mario64") && state.rawAction == 0x1371));
    };
    for (int i = 0; i < 30; i++)
        step();
    assert(state.flags & SC_GROUNDED);
    float ground = state.position[1], top = ground;
    for (int i = 0; i < 70; i++) {
        input.jump = i < 8;
        step();
        top = std::max(top, state.position[1]);
    }
    assert(top - ground > 10);
    assert(fabsf(state.position[1] - ground) < 5);
    printf("%s: mesh=%u jump=%.2f cm\n", d->id, m.numTrianglesUsed, top - ground);
    input.jump = 1;
    step();
    float velocityY = state.velocity[1];
    assert(velocityY > 0);
    auto rawAction = state.rawAction;
    float vertexX = m.position[0];
    SC_MotionResolution resolution = {};
    resolution.size = sizeof(resolution);
    resolution.generation = 2;
    resolution.sequence = 7;
    for (int k = 0; k < 3; k++)
        resolution.position[k] = state.position[k];
    resolution.position[0] += 12;
    resolution.normalCount = 1;
    resolution.normals[0][1] = -1;
    assert(a->resolveMotion(id, &resolution, &state, &m));
    assert(SC::frame(state, m, *d));
    assert(state.velocity[1] == 0);
    assert(state.rawAction == rawAction);
    assert(fabsf(m.position[0] - vertexX - 12) < .01f);
    resolution.normalCount = 5;
    assert(!a->resolveMotion(id, &resolution, &state, &m));
    a->resume(id, 0, 0, 0, 0);
    input.jump = 0;
    input.moveY = 1;
    for (int i = 0; i < 45; i++)
        step();
    assert(fabsf(state.position[2]) > 50);
    // A host origin rebase updates its mesh/state as well as the guest position.
    // The following acknowledgement must not apply the old origin displacement twice.
    float shift[3] = {1000, 0, 2000};
    for (int k = 0; k < 3; k++) {
        state.position[k] -= shift[k];
        for (unsigned j = 0; j < m.numTrianglesUsed * 3; j++)
            m.position[j * 3 + k] -= shift[k];
    }
    a->setPosition(id, state.position[0], state.position[1], state.position[2]);
    float rebasedVertex = m.position[0];
    resolution.normalCount = 0;
    for (int k = 0; k < 3; k++)
        resolution.position[k] = state.position[k];
    assert(a->resolveMotion(id, &resolution, &state, &m));
    assert(fabsf(m.position[0] - rebasedVertex) < .01f);
    // Engine-specific regression: a no-contact acknowledgement between a paid
    // Mario stomp and the next tick must preserve the engine's bounce velocity.
    if (!strcmp(d->id, "mario64")) {
        a->resume(id, 0, 300, 0, 0);
        a->command(id, SC_COMMAND_FALL);
        input = {0, 1, 0, 0, 0, 0, 0, 0};
        step();
        assert(state.velocity[1] < 0);
        assert(a->probeContact(id, 0, 0, 0) & SC_CONTACT_AUTOMATIC);
        a->commitContact(id, 0, 0, 0, 160);
        for (int k = 0; k < 3; k++)
            resolution.position[k] = state.position[k];
        assert(a->resolveMotion(id, &resolution, &state, &m));
        step();
        assert(state.velocity[1] > 0);
        // A terminal floor is still queryable by engines that require a floor,
        // but the host must detect contact before a protected landing can continue.
        SC_Surface terminal[2] = {floor[0], floor[1]};
        for (auto &t : terminal)
            for (auto &v : t.vertices)
                v[1] = -1500;
        a->surfaces(terminal, 2, 0);
        a->resume(id, 0, 0, 0, 0);
        a->command(id, SC_COMMAND_FALL);
        input = {0, 1, 0, 0, 0, 0, 0, 0};
        SCHazard::Triangle kill = {{{-160, -15, -160}, {-160, -15, 160}, {160, -15, 160}}};
        SCHazard::Point previous = {0, 0, 0};
        bool terminalContact = false;
        for (int i = 0; i < 180; i++) {
            a->nativeDamage(id, 0);
            step();
            SCHazard::Point next = {state.position[0] * .01f, state.position[1] * .01f,
                                    state.position[2] * .01f};
            if (SCHazard::touches(previous, next, kill)) {
                terminalContact = true;
                break;
            }
            previous = next;
        }
        assert(terminalContact);
        assert(d->body.nonlethalFalls);
        puts(
            "PASS real guest fall reaches host lethal surface despite nonlethal landing protection");
        // Native-owned floors may be absent from the guest static cache. A missing
        // floor must allow motion/freefall, not fabricate an invisible boundary wall.
        SC_Surface edge[2] = {{{{-1000, 0, -200}, {-1000, 0, 200}, {1000, 0, 200}}, 0},
                              {{{-1000, 0, -200}, {1000, 0, 200}, {1000, 0, -200}}, 0}};
        a->surfaces(edge, 2, 0);
        a->surfaces(nullptr, 0, 1);
        a->resume(id, 0, 0, 0, 0);
        input = {0, 1, 0, 1, 0, 0, 0, 0};
        for (int i = 0; i < 45; i++)
            step();
        printf("missing-floor travel z=%.2f y=%.2f\n", state.position[2], state.position[1]);
        assert(fabsf(state.position[2]) > 400);
        assert(state.position[1] < -50);
        assert(!(state.flags & SC_GROUNDED));
        assert(a->floorHeight(0, 100, 1000) < -10000); // public probe still reports no real floor
        // Native collision can then correct onto its currently enabled floor.
        a->resume(id, 0, 0, 0, 0);
        SC_MotionResolution support = {};
        support.size = sizeof(support);
        support.generation = 3;
        for (int i = 0; i < 60; i++) {
            step();
            for (int axis = 0; axis < 3; axis++)
                support.position[axis] = state.position[axis];
            support.position[1] = 0;
            support.normalCount = 1;
            support.normals[0][1] = 1;
            support.sequence = i + 1;
            assert(a->resolveMotion(id, &support, &state, &m));
            int z = int(state.position[2]);
            SC_Surface patch[2] = {{{{-42, 0, z - 42}, {-42, 0, z + 42}, {42, 0, z + 42}}, 0},
                                   {{{-42, 0, z - 42}, {42, 0, z + 42}, {42, 0, z - 42}}, 0}};
            a->surfaces(patch, 2, 1);
        }
        assert(fabsf(state.position[2]) > 800);
        assert(fabsf(state.position[1]) < 1);
        a->surfaces(nullptr, 0, 1);
        puts("PASS missing guest floor: free movement/freefall and native floor contact handoff");
        // Real guest collision queries must match the descriptor, not retain the
        // old 50 cm wall radius / 160 cm ceiling height beneath a smaller host body.
        SC_Surface walls[6] = {floor[0],
                               floor[1],
                               {{{-1000, 0, 200}, {-1000, 1000, 200}, {1000, 1000, 200}}, 0},
                               {{{-1000, 0, 200}, {1000, 1000, 200}, {1000, 0, 200}}, 0},
                               {{{-1000, 0, -200}, {1000, 1000, -200}, {-1000, 1000, -200}}, 0},
                               {{{-1000, 0, -200}, {1000, 0, -200}, {1000, 1000, -200}}, 0}};
        a->surfaces(walls, 6, 0);
        a->resume(id, 0, 0, 0, 0);
        input = {0, 1, 0, 1, 0, 0, 0, 0};
        for (int i = 0; i < 70; i++)
            step();
        printf("body wall clearance: %.2f cm (profile %.2f)\n", 200 - fabsf(state.position[2]),
               d->body.radiusCm);
        assert(fabsf(200 - fabsf(state.position[2]) - d->body.radiusCm) < 1);
        SC_Surface tunnel[4] = {floor[0],
                                floor[1],
                                {{{-1000, 120, -1000}, {1000, 120, 1000}, {-1000, 120, 1000}}, 0},
                                {{{-1000, 120, -1000}, {1000, 120, -1000}, {1000, 120, 1000}}, 0}};
        a->surfaces(tunnel, 4, 0);
        a->resume(id, 0, 0, 0, 0);
        for (int i = 0; i < 20; i++)
            step();
        assert(fabsf(state.position[2]) > 100);
        assert(state.flags & SC_GROUNDED);
        a->resume(id, 0, 0, 0, 0);
        input = {0, 1, 0, 0, 0, 0, 0, 0};
        step();
        float ceilingTop = 0;
        for (int i = 0; i < 35; i++) {
            input.jump = i < 8;
            step();
            ceilingTop = std::max(ceilingTop, state.position[1]);
        }
        assert(ceilingTop <= 120 - d->body.heightCm + 1);
        printf("body tunnel: walks under 120 cm ceiling, jump peak %.2f cm\n", ceilingTop);
        a->surfaces(floor, 2, 0);
        input = {0, 1, 0, 0, 0, 0, 0, 0};
    }

    if (auto getLauncher = (SC_GetLauncherAPI)GetProcAddress(dll, "SoulsCharacterGetLauncherAPI")) {
        assert(!getLauncher(99, sizeof(SC_LauncherAPI)));
        const auto *l = getLauncher(SC_LAUNCHER_VERSION, sizeof(SC_LauncherAPI));
        assert(l);
        std::vector<uint8_t> lt(l->textureWidth * l->textureHeight * 4);
        SC_Texture tx = {lt.data(), uint32_t(lt.size())};
        assert(l->texture(&tx));
        for (unsigned part = 1; part <= 4; part++) {
            assert(l->mesh(part, 12, 0, &m));
            assert(m.numTrianglesUsed > 0);
            printf("launcher prop %u triangles %u\n", part, m.numTrianglesUsed);
        }
        for (int frame = 0; frame < 150; frame++) {
            assert(l->mesh(SC_LAUNCHER_OPERATOR, float(frame), frame * .04f, &m));
            assert(m.numTrianglesUsed <= m.capacityTriangles);
            for (unsigned v = 0; v < m.numTrianglesUsed * 9; v++)
                assert(std::isfinite(m.position[v]) && fabsf(m.position[v]) < 200);
        }
        SC_Mesh small = m;
        small.capacityTriangles = 1;
        assert(!l->mesh(SC_LAUNCHER_OPERATOR, 0, 0, &small));
        assert(!l->mesh(99, 0, 0, &m));
        assert(!l->mesh(1, NAN, 0, &m));
        a->resume(id, 0, 0, 0, 0);
        input = {0, 1, 0, 0, 0, 0, 0, 0};
        step();
        float at[3] = {0, 0, 0};
        assert(l->begin(id, at, 0));
        assert(!l->begin(id, at, 0));
        SC_LauncherState ls = {sizeof(ls)};
        input.jump = 1;
        for (int i = 0; i < 160; i++) {
            step();
            assert(l->query(id, &ls));
            assert(ls.phase != SC_LAUNCHER_FLIGHT);
        }
        assert(ls.phase == SC_LAUNCHER_AIMING);
        float initialYaw = ls.yaw;
        input.jump = 0;
        step();
        input.moveY = 1;
        for (int i = 0; i < 60; i++)
            step();
        assert(l->query(id, &ls));
        printf("launcher pitch %.5f\n", ls.pitch);
        assert(fabsf(ls.pitch - 0x38e3 * 3.14159265359f / 32768) < .001f);
        input.moveY = 0;
        input.moveX = 1;
        for (int i = 0; i < 70; i++)
            step();
        assert(l->query(id, &ls));
        printf("launcher yaw %.5f\n", ls.yaw);
        assert(fabsf(fabsf(ls.yaw - initialYaw) - 1.5707963f) < .001f);
        input.moveX = 0;
        input.jump = 1;
        step();
        assert(l->query(id, &ls) && ls.phase == SC_LAUNCHER_FLIGHT);
        assert(fabsf(state.velocity[1] - 3000 * sinf(ls.pitch)) < 2);
        input.jump = 0;
        float vy = state.velocity[1];
        step();
        assert(fabsf(state.velocity[1] - (vy - 30)) < .1f);
        l->cancel(id);
        a->resume(id, 0, 0, 0, 0);
        step();
        assert(m.numTrianglesUsed > 0);
        assert(l->begin(id, at, 0));
        step();
        a->nativeDamage(id, 1);
        step();
        assert(l->query(id, &ls) && !ls.phase);
        assert(m.numTrianglesUsed > 0);
        printf(
            "PASS real cannon opening/aim limits/held-A gate/launch speed/gravity/hurt cancellation\n");
        float headings[2] = {};
        for (int shot = 0; shot < 2; shot++) {
            l->cancel(id);
            a->resume(id, 0, 0, 0, 0);
            input = {0, 1, 0, 0, 0, 0, 0, 0};
            step();
            assert(l->begin(id, at, 0));
            for (int frame = 0; frame < 160; frame++) {
                a->nativeDamage(id, 0);
                step();
            }
            assert(l->query(id, &ls) && ls.phase == SC_LAUNCHER_AIMING);
            input.moveX = shot ? 1.f : -1.f;
            input.moveY = .5f;
            for (int frame = 0; frame < 12; frame++)
                step();
            input.moveX = input.moveY = 0;
            step();
            assert(l->query(id, &ls));
            const auto aimed = ls;
            headings[shot] = ls.yaw;
            // Reconcile exactly as the host does between simulation ticks.
            SC_MotionResolution ack = {};
            ack.size = sizeof(ack);
            memcpy(ack.position, state.position, sizeof(ack.position));
            assert(a->resolveMotion(id, &ack, &state, &m));
            input.jump = 1;
            step();
            assert(l->query(id, &ls) && ls.phase == SC_LAUNCHER_FLIGHT);
            assert(fabsf(ls.yaw - aimed.yaw) < .001f && fabsf(ls.pitch - aimed.pitch) < .001f);
            assert(fabsf(state.position[0] - aimed.position[0] -
                         120 * cosf(aimed.pitch) * sinf(aimed.yaw)) < 1);
            assert(fabsf(state.position[2] - aimed.position[2] -
                         120 * cosf(aimed.pitch) * cosf(aimed.yaw)) < 1);
            input.jump = 0;
            step();
            assert(fabsf(state.velocity[0] - 3000 * cosf(aimed.pitch) * sinf(aimed.yaw)) < 4);
            assert(fabsf(state.velocity[2] - 3000 * cosf(aimed.pitch) * cosf(aimed.yaw)) < 4);
        }
        assert(headings[1] - headings[0] > .5f); // screen-right increases yaw in the host frame
        l->cancel(id);
        a->resume(id, 0, 0, 0, 0);
        input = {0, 1, 0, 0, 0, 0, 0, 0};
        step();
        puts(
            "PASS cannon aim/launch agreement: opposite yaw inputs, release, motion acknowledgement, muzzle and flight vectors");
    }
    for (int kind = SC_POSE_NEUTRAL; kind <= SC_POSE_STAGGER; kind++)
        for (int frame = 0; frame < 20; frame++) {
            SC_Pose p = {uint32_t(kind), {100, 50, 200}, .75f, frame / 19.f, float(frame * 3)};
            assert(a->pose(id, &p, &state, &m));
            assert(SC::frame(state, m, *d));
            assert(fabsf(state.position[1] - 50) < .01f);
        }
    a->resume(id, 0, 0, 0, 0);
    input = {0, 1, 0, 0, 0, 0, 0, 0};
    step();
    input.jump = 1;
    step();
    assert(state.position[1] > 0);
    input.jump = 0;
    int16_t tiny[2] = {123, 456};
    assert(a->audio(0, 1100, tiny, 1) == 0);
    assert(tiny[0] == 123 && tiny[1] == 456);
    int16_t pcm[2176];
    assert(a->audio(0, 1100, pcm, 1088) <= 1088);
    a->nativeDamage(id, 1);
    step();
    a->nativeDamage(id, 0);
    SC_Mesh tooSmall = m;
    tooSmall.capacityTriangles = 0;
    assert(!a->step(id, &input, &state, &tooSmall));
    a->destroy(id);
    assert(!a->step(id, &input, &state, &m));
    id = a->create(0, 30, 0, 0);
    assert(id >= 0);
    for (int i = 0; i < 30; i++)
        step();
    a->destroy(id);
    a->shutdown();
    FreeLibrary(dll);
    puts(
        "PASS adapter contract: version rejection, initialization, geometry, movement, jump, poses, same-instance resume, collision correction, origin rebase, bounce preservation, audio bounds, damage callback, destroy/recreate");
}
