#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../../sdk/character_api.h"

static void require(bool value, const char *message) {
    if (!value) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void logMessage(const char *message) {
    fprintf(stderr, "adapter: %s\n", message);
}

static bool closeFloat(float a, float b) {
    return std::isfinite(a) && std::isfinite(b) && fabsf(a - b) < .001f;
}

static void rigid(const float *m) {
    for (int i = 0; i < 16; i++)
        require(std::isfinite(m[i]), "finite socket");
    require(closeFloat(m[3], 0) && closeFloat(m[7], 0) && closeFloat(m[11], 0) &&
                closeFloat(m[15], 1),
            "homogeneous rigid socket");
    for (int a = 0; a < 3; a++)
        for (int b = 0; b < 3; b++) {
            float d = 0;
            for (int k = 0; k < 3; k++)
                d += m[a * 4 + k] * m[b * 4 + k];
            require(closeFloat(d, a == b ? 1.f : 0.f), "orthonormal socket");
        }
}

struct Buffers {
    std::vector<float> positions, normals, colors, uv;
    SC_Mesh mesh;

    Buffers(unsigned n)
        : positions(n * 9), normals(n * 9), colors(n * 9), uv(n * 6),
          mesh{positions.data(), normals.data(), colors.data(), uv.data(), n, 0} {}
};

struct Frame {
    SC_State state;
    std::vector<float> geometry;
};

static SC_Input input(unsigned frame) {
    SC_Input in{};
    in.camLookZ = 1;
    if (frame >= 25 && frame < 140)
        in.moveY = 1;
    if (frame >= 50 && frame < 56)
        in.jump = 1;
    if (frame >= 108 && frame < 112)
        in.attack = 1;
    if (frame >= 160 && frame < 165)
        in.attack = 1;
    return in;
}

static std::vector<Frame> run(const SC_CharacterAPI *api, const char *asset, const char *runtime,
                              uint32_t kind) {
    bool equipped = kind != 0;
    const SC_Descriptor *d = api->describe();
    std::vector<uint8_t> texture(d->textureWidth * d->textureHeight * 4);
    SC_Texture tex = {texture.data(), (uint32_t)texture.size()};
    SC_Init init = {sizeof(init), asset, runtime, logMessage};
    require(api->initialize(&init, &tex), "initialize owned Mario runtime");
    SC_Surface surfaces[2] = {{{{-16000, 0, -16000}, {-16000, 0, 16000}, {16000, 0, 16000}}, 0},
                              {{{-16000, 0, -16000}, {16000, 0, 16000}, {16000, 0, -16000}}, 0}};
    api->surfaces(surfaces, 2, 0);
    int id = api->create(0, 0, 0, 0);
    require(id >= 0, "create Mario");
    Buffers b(d->maxTriangles);
    std::vector<Frame> frames;
    unsigned sockets = 0;
    // The pinned renderer starts its global animation counter at zero, which
    // can suppress its very first frame. Compare both runs after the same
    // explicit locomotion resume boundary rather than that startup sentinel.
    SC_Input warmup{};
    warmup.camLookZ = 1;
    SC_State warmState{};
    api->step(id, &warmup, &warmState, &b.mesh);
    api->resume(id, 0, 0, 0, 0);
    for (unsigned i = 0; i < 190; i++) {
        SC_EquipmentPose equipment = {sizeof(equipment),
                                      kind,
                                      equipped ? SC_EQUIPMENT_SHIELD : SC_EQUIPMENT_NONE,
                                      equipped ? (i / 23) % 2 + 1 : 0,
                                      (i % 23) / 22.f,
                                      (i % 31) / 30.f};
        require(api->equipment(id, &equipment), "accept valid equipment presentation");
        // Match the host order: presentation, authoritative-health maintenance,
        // then the simulation tick. Zero severity is not an incoming hit.
        api->nativeDamage(id, 0);
        SC_Input in = input(i);
        Frame f{};
        require(api->step(id, &in, &f.state, &b.mesh), "paired simulation step");
        if (!(f.state.flags & SC_GROUNDED))
            require(!(f.state.flags & SC_CAN_EQUIP_ACTION),
                    "airborne movement cannot start equipment actions");
        if (f.state.attackId >= 0)
            require(!(f.state.flags & SC_CAN_EQUIP_ACTION),
                    "guest attack cannot start equipment action");
        if (i == 10)
            require(f.state.flags & SC_CAN_EQUIP_ACTION,
                    "ordinary grounded idle can start equipment action");
        SC_Attachments a = {sizeof(a)};
        require(api->attachments(id, &a), "read attachment snapshot");
        if (equipped && a.valid) {
            require(a.valid == 3, "both equipped hands available");
            rigid(a.right);
            rigid(a.left);
            sockets++;
        }
        if (equipped && i == 10) {
            require(a.right[12] > f.state.position[0] + 5 && a.left[12] < f.state.position[0] - 5,
                    "native-right sword and native-left shield follow rendered glove handedness");
        }
        if (!equipped)
            require(!a.valid, "no attachments when unequipped");
        f.geometry.assign(b.positions.begin(), b.positions.begin() + b.mesh.numTrianglesUsed * 9);
        frames.push_back(std::move(f));
    }
    if (equipped) {
        require(sockets > 150, "equipment sockets through run/jump/swing/guard phases");
        SC_State state = frames.back().state;
        SC_Attachments before = {sizeof(before)}, after = {sizeof(after)};
        api->attachments(id, &before);
        require(before.valid == 3, "correction fixture socket");
        api->nativeDamage(id, 0);
        api->attachments(id, &after);
        require(!memcmp(&before, &after, sizeof(before)),
                "zero-damage health maintenance preserves current pose and sockets");
        SC_MotionResolution correction = {};
        correction.size = sizeof(correction);
        const float delta[] = {13, -2, 19};
        for (int k = 0; k < 3; k++)
            correction.position[k] = state.position[k] + delta[k];
        std::vector<float> meshBefore = b.positions;
        require(api->resolveMotion(id, &correction, &state, &b.mesh), "host motion correction");
        api->attachments(id, &after);
        for (int k = 0; k < 3; k++) {
            require(closeFloat(after.right[12 + k] - before.right[12 + k], delta[k]) &&
                        closeFloat(after.left[12 + k] - before.left[12 + k], delta[k]),
                    "sockets follow host correction");
            for (unsigned v = 0; v < b.mesh.numTrianglesUsed * 3; v++)
                require(closeFloat(b.positions[v * 3 + k] - meshBefore[v * 3 + k], delta[k]),
                        "mesh follows identical host correction");
        }
        before = after;
        api->setPosition(id, state.position[0] + 5, state.position[1] + 6, state.position[2] + 7);
        api->attachments(id, &after);
        for (int k = 0; k < 3; k++)
            require(closeFloat(after.right[12 + k] - before.right[12 + k], 5.f + k) &&
                        closeFloat(after.left[12 + k] - before.left[12 + k], 5.f + k),
                    "setPosition rebases both sockets");
        SC_EquipmentPose invalid = {
            sizeof(invalid), SC_EQUIPMENT_SWORD, SC_EQUIPMENT_SHIELD, 1, NAN, 0};
        require(!api->equipment(id, &invalid), "reject nonfinite phase");
        SC_Pose pose = {};
        pose.kind = SC_POSE_DRINK;
        pose.phase = .6f;
        require(api->pose(id, &pose, &state, &b.mesh), "native action presentation");
        api->attachments(id, &after);
        require(!after.valid, "interaction suppresses equipment sockets");
        api->resume(id, 0, 0, 0, 0);
        api->attachments(id, &after);
        require(!after.valid, "resume clears stale equipment pose");
        SC_EquipmentPose equip = {sizeof(equip), SC_EQUIPMENT_SWORD, SC_EQUIPMENT_SHIELD, 0, 0, 1};
        SC_Input in{};
        in.camLookZ = 1;
        api->equipment(id, &equip);
        api->step(id, &in, &state, &b.mesh);
        api->nativeDamage(id, 1);
        api->attachments(id, &after);
        require(!after.valid, "damage clears stale equipment pose");
        api->equipment(id, &equip);
        api->step(id, &in, &state, &b.mesh);
        api->attachments(id, &after);
        require(!after.valid,
                "hurt reaction suppresses overlay even if host still requests equipment");
        require(!(state.flags & SC_CAN_EQUIP_ACTION),
                "hurt reaction cannot start equipment action");
    }
    api->destroy(id);
    api->shutdown();
    return frames;
}

static void nativeCombatPoses(const SC_CharacterAPI *api, const char *asset, const char *runtime) {
    auto *d = api->describe();
    std::vector<uint8_t> pixels(d->textureWidth * d->textureHeight * 4);
    SC_Texture texture = {pixels.data(), (uint32_t)pixels.size()};
    SC_Init init = {sizeof(init), asset, runtime, logMessage};
    require(api->initialize(&init, &texture), "combat pose initialize");
    SC_Surface floor[2] = {{{{-16000, 0, -16000}, {-16000, 0, 16000}, {16000, 0, 16000}}, 0},
                           {{{-16000, 0, -16000}, {16000, 0, 16000}, {16000, 0, -16000}}, 0}};
    api->surfaces(floor, 2, 0);
    int id = api->create(0, 0, 0, 0);
    require(id >= 0, "native combat pose create");
    Buffers b(d->maxTriangles);
    SC_State state{};
    for (unsigned kind : {0u, 1u, 3u, 4u, 5u, 6u, 7u})
        for (unsigned action : {3u, 4u, 5u}) {
            if (kind == 0 && action != SC_EQUIPMENT_PARRY)
                continue;
            std::vector<float> first;
            unsigned changed = 0;
            for (int frame = 0; frame <= 30; frame++) {
                SC_EquipmentPose e = {sizeof(e), kind,         SC_EQUIPMENT_SHIELD,
                                      action,    frame / 30.f, 0};
                require(api->equipment(id, &e), "critical/parry request");
                SC_Pose pose = {};
                pose.kind = SC_POSE_NATIVE_COMBAT;
                pose.phase = -1;
                pose.position[0] = 17;
                pose.position[1] = 31;
                pose.position[2] = 53;
                require(api->pose(id, &pose, &state, &b.mesh), "native combat pose");
                require(state.attackId == -1 && state.flags == 0,
                        "presentation cannot start guest attack or navigation");
                require(closeFloat(state.position[0], 17) && closeFloat(state.position[1], 31) &&
                            closeFloat(state.position[2], 53),
                        "native root authority");
                SC_Attachments hands = {sizeof(hands)};
                require(api->attachments(id, &hands) && hands.valid == (kind ? 3u : 2u),
                        "native combat preserves equipped props");
                if (kind)
                    rigid(hands.right);
                rigid(hands.left);
                if (frame == 0)
                    first = b.positions;
                else if (first != b.positions)
                    changed++;
            }
            require(changed > 20, "native combat visibly animates");
        }
    api->destroy(id);
    api->shutdown();
    puts(
        "PASS native combat poses: all families, preserved sockets, no guest attacks, host root position");
}

static void armedIdle(const SC_CharacterAPI *api, const char *asset, const char *runtime) {
    auto *d = api->describe();
    std::vector<uint8_t> pixels(d->textureWidth * d->textureHeight * 4);
    SC_Texture texture = {pixels.data(), (uint32_t)pixels.size()};
    SC_Init init = {sizeof(init), asset, runtime, logMessage};
    require(api->initialize(&init, &texture), "idle policy initialize");
    SC_Surface floor[2] = {{{{-16000, 0, -16000}, {-16000, 0, 16000}, {16000, 0, 16000}}, 0},
                           {{{-16000, 0, -16000}, {16000, 0, 16000}, {16000, 0, -16000}}, 0}};
    api->surfaces(floor, 2, 0);
    int id = api->create(0, 0, 0, 0);
    require(id >= 0, "idle policy create");
    Buffers b(d->maxTriangles);
    SC_State state{};
    SC_Input input{};
    input.camLookZ = 1;
    SC_EquipmentPose equipment = {sizeof(equipment)};
    auto tick = [&]() {
        require(api->equipment(id, &equipment), "idle equipment request");
        api->nativeDamage(id, 0);
        require(api->step(id, &input, &state, &b.mesh), "idle simulation tick");
    };
    unsigned idleTicks = 0;
    for (; idleTicks < 6000; idleTicks++) {
        tick();
        if (state.rawAction == 0x0c000203)
            break;
    }
    require(idleTicks < 6000, "unarmed Mario retains original sleeping behavior");
    equipment.rightKind = SC_EQUIPMENT_SWORD;
    equipment.leftKind = SC_EQUIPMENT_SHIELD;
    tick();
    require(state.rawAction == 0x0c000204 && !(state.flags & SC_CAN_EQUIP_ACTION),
            "equipping while asleep uses native wake transition");
    unsigned wakeTicks = 0;
    for (; wakeTicks < 40; wakeTicks++) {
        tick();
        if (state.flags & SC_CAN_EQUIP_ACTION)
            break;
    }
    require(wakeTicks < 40 && state.rawAction == 0x0c400201,
            "equipment actions become eligible after waking");
    for (unsigned i = 0; i < 6000; i++) {
        tick();
        require(state.rawAction == 0x0c400201 && (state.flags & SC_CAN_EQUIP_ACTION),
                "armed long idle remains ready without forced locomotion");
        require(closeFloat(state.position[0], 0) && closeFloat(state.position[1], 0) &&
                    closeFloat(state.position[2], 0),
                "armed idle preserves position");
    }
    equipment.rightKind = equipment.leftKind = 0;
    unsigned sleepTicks = 0;
    for (; sleepTicks < 6000; sleepTicks++) {
        tick();
        if (state.rawAction == 0x0c000203)
            break;
    }
    require(sleepTicks < 6000, "unequipping restores normal sleep");
    printf(
        "Armed idle: original sleep after %u ticks, native wake in %u ticks, 6000 armed ticks ready, unequipped sleep restored\n",
        idleTicks, wakeTicks);
    api->destroy(id);
    api->shutdown();
}

/* Optional private visual evidence; geometry originates in the user's ROM and
 * must not be checked in. Header: frame count, then triangle count + phase +
 * guard amount, right/left matrices, triangle positions and RGB per frame. */
static void preview(const SC_CharacterAPI *api, const char *asset, const char *runtime,
                    const char *path) {
    auto *d = api->describe();
    std::vector<uint8_t> t(d->textureWidth * d->textureHeight * 4);
    SC_Texture texture = {t.data(), (uint32_t)t.size()};
    SC_Init init = {sizeof(init), asset, runtime, logMessage};
    require(api->initialize(&init, &texture), "preview initialize");
    SC_Surface surfaces[2] = {{{{-16000, 0, -16000}, {-16000, 0, 16000}, {16000, 0, 16000}}, 0},
                              {{{-16000, 0, -16000}, {16000, 0, 16000}, {16000, 0, -16000}}, 0}};
    api->surfaces(surfaces, 2, 0);
    int id = api->create(0, 0, 0, 0);
    Buffers b(d->maxTriangles);
    SC_State state{};
    SC_Input in{};
    in.camLookZ = 1;
    for (int i = 0; i < 30; i++)
        api->step(id, &in, &state, &b.mesh);
    FILE *f = fopen(path, "wb");
    require(f != nullptr, "preview output");
    uint32_t count = 27;
    fwrite(&count, 4, 1, f);
    for (unsigned frame = 0; frame < count; frame++) {
        SC_EquipmentPose e = {sizeof(e),
                              SC_EQUIPMENT_SWORD,
                              SC_EQUIPMENT_SHIELD,
                              frame < 23 ? 1u : 0u,
                              frame < 23 ? frame / 22.f : 0.f,
                              frame < 23 ? 0.f : (frame - 23) / 3.f};
        api->equipment(id, &e);
        api->step(id, &in, &state, &b.mesh);
        SC_Attachments a = {sizeof(a)};
        api->attachments(id, &a);
        require(a.valid == 3, "preview sockets");
        fwrite(&b.mesh.numTrianglesUsed, 4, 1, f);
        fwrite(&e.swingPhase, 4, 1, f);
        fwrite(&e.guard, 4, 1, f);
        fwrite(a.right, 4, 16, f);
        fwrite(a.left, 4, 16, f);
        fwrite(b.mesh.position, 4, b.mesh.numTrianglesUsed * 9, f);
        fwrite(b.mesh.color, 4, b.mesh.numTrianglesUsed * 9, f);
    }
    fclose(f);
    api->destroy(id);
    api->shutdown();
}

#include "fire_flower_test.h"

int main(int argc, char **argv) {
    if (argc != 4 && argc != 5) {
        fprintf(
            stderr,
            "usage: equipment_test character.dll owned-rom.z64 sm64.dll [private-preview.bin]\n");
        return 2;
    }
    HMODULE module = LoadLibraryA(argv[1]);
    require(module != nullptr, "load adapter");
    auto get = (SC_GetAPI)GetProcAddress(module, "SoulsCharacterGetAPI");
    require(get != nullptr, "adapter entrypoint");
    const SC_CharacterAPI *api = get(SC_ABI_VERSION, sizeof(SC_CharacterAPI));
    require(api && api->equipment && api->attachments, "equipment ABI");
    auto baseline = run(api, argv[2], argv[3], 0);
    for (uint32_t kind : {1u, 3u, 4u, 5u, 6u, 7u}) {
        auto equipped = run(api, argv[2], argv[3], kind);
        unsigned changed = 0;
        require(baseline.size() == equipped.size(), "paired trace lengths");
        for (unsigned i = 0; i < baseline.size(); i++) {
            const SC_State &a = baseline[i].state, &b = equipped[i].state;
            if (!(a.rawAction == b.rawAction && a.rawAnimation == b.rawAnimation &&
                  a.rawAnimationFrame == b.rawAnimationFrame && a.rawFlags == b.rawFlags &&
                  a.attackId == b.attackId && a.flags == b.flags))
                fprintf(
                    stderr,
                    "frame %u: action %08x/%08x anim %d:%d/%d:%d rawFlags %08x/%08x attack %d/%d flags %u/%u\n",
                    i, a.rawAction, b.rawAction, a.rawAnimation, a.rawAnimationFrame,
                    b.rawAnimation, b.rawAnimationFrame, a.rawFlags, b.rawFlags, a.attackId,
                    b.attackId, a.flags, b.flags);
            require(a.rawAction == b.rawAction && a.rawAnimation == b.rawAnimation &&
                        a.rawAnimationFrame == b.rawAnimationFrame && a.rawFlags == b.rawFlags &&
                        a.attackId == b.attackId && a.flags == b.flags,
                    "equipment does not change action or animation timing");
            for (int k = 0; k < 3; k++)
                require(closeFloat(a.position[k], b.position[k]) &&
                            closeFloat(a.velocity[k], b.velocity[k]),
                        "equipment does not change movement");
            require(closeFloat(a.floorHeight, b.floorHeight) &&
                        closeFloat(a.faceAngle, b.faceAngle),
                    "equipment preserves floor and facing");
            require(baseline[i].geometry.size() == equipped[i].geometry.size(),
                    "equipment preserves Mario topology");
            // Visible foot planting must survive the new torso/arm overlay.
            if (a.rawAction == 0x0c400201)
                for (unsigned v = 0; v < baseline[i].geometry.size() / 3; v++) {
                    if (baseline[i].geometry[v * 3 + 1] < a.position[1] + 20)
                        for (int axis = 0; axis < 3; axis++)
                            require(closeFloat(baseline[i].geometry[v * 3 + axis],
                                               equipped[i].geometry[v * 3 + axis]),
                                    "weapon windup keeps planted feet unchanged");
                }
            if (baseline[i].geometry != equipped[i].geometry)
                changed++;
        }
        require(changed > 150, "rendered arms actually change");
        printf("Melee kind %u: movement parity and rigid attachments passed\n", kind);
    }
    fireFlowerTest(api, argv[2], argv[3]);
    nativeCombatPoses(api, argv[2], argv[3]);
    armedIdle(api, argv[2], argv[3]);
    if (argc == 5)
        preview(api, argv[2], argv[3], argv[4]);
    FreeLibrary(module);
    puts(
        "PASS: Mario equipment movement/action parity, rendered arm poses, finite rigid sockets, correction/rebase, interaction/hurt lifecycle, armed-idle wake policy");
    return 0;
}
