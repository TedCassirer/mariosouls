#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../sdk/character_api.h"
#include "../../sdk/launcher_api.h"
#include "../../core/motion.h"
#include "../../../work/libsm64/src/libsm64.h"
#include "profile.generated.h"

// The SDK allows one live guest instance. Keep its transient state separate from
// the runtime/ROM lifetime so destruction and recreation do not unload assets.
#include "runtime_library.h"
#include "actions.h"

struct AdapterResources {
    bool initialized = false;
    SC_Log log = nullptr;
    std::vector<uint8_t> ownedRom;
    bool cannonAssetsReady = false;
};

struct MarioInstance {
    int32_t id = -1;
    SM64MarioState engineState{};
    uint32_t previousAction = 0;
    SC_Attachments attachments = {sizeof(SC_Attachments)};
    SC_EquipmentPose equipmentPose = {sizeof(SC_EquipmentPose)};
    bool fireAppearance = false;
};

static MarioRuntimeLibrary library;
static AdapterResources adapter;
static MarioInstance mario;
#include "cannon_assets.h"
static void firePresentation(SC_Mesh &, bool);

enum Move { None = -1, Punch1, Punch2, Kick, JumpKick, Dive, SlideKick, Stomp, Pound };

static int identifyAttack(uint32_t action, unsigned argument) {
    if (action == MarioAction::ACT_PUNCHING || action == MarioAction::ACT_MOVE_PUNCHING) {
        if (argument <= 2)
            return Punch1;
        if (argument <= 5)
            return Punch2;
        if (argument == 6)
            return Kick;
        if (argument == 9)
            return SlideKick;
        return None;
    }
    if (action == MarioAction::ACT_JUMP_KICK)
        return JumpKick;
    if (action == MarioAction::ACT_DIVE || action == MarioAction::ACT_DIVE_SLIDE)
        return Dive;
    if (action == MarioAction::ACT_SLIDE_KICK || action == MarioAction::ACT_SLIDE_KICK_SLIDE)
        return SlideKick;
    if (action == MarioAction::ACT_GROUND_POUND || action == MarioAction::ACT_GROUND_POUND_LAND)
        return Pound;
    return None;
}

static bool equipmentActionEligible(uint32_t action) {
    // Explicit ordinary locomotion allowlist. A non-air action alone is not a
    // safe indication: damage, burning, scripted states and recovery also land
    // on the floor. Keep that guest taxonomy inside its adapter.
    switch (action) {
    case MarioAction::ACT_IDLE:
    case MarioAction::ACT_PANTING:
    case MarioAction::ACT_STANDING_AGAINST_WALL: // idle, panting, wall
    case MarioAction::ACT_WALKING:
    case MarioAction::ACT_TURNING_AROUND:
    case MarioAction::ACT_FINISH_TURNING_AROUND: // walk and turn
    case MarioAction::ACT_BRAKING:
    case MarioAction::ACT_DECELERATING:
    case MarioAction::ACT_BRAKING_STOP: // brake/decelerate
    case MarioAction::ACT_JUMP_LAND_STOP:
    case MarioAction::ACT_DOUBLE_JUMP_LAND_STOP:
    case MarioAction::ACT_FREEFALL_LAND_STOP:
    case MarioAction::ACT_SIDE_FLIP_LAND_STOP:
    case MarioAction::ACT_JUMP_LAND:
    case MarioAction::ACT_FREEFALL_LAND:
    case MarioAction::ACT_DOUBLE_JUMP_LAND:
    case MarioAction::ACT_SIDE_FLIP_LAND:
        return true;
    default:
        return false;
    }
}

static void writeFrameworkState(SC_State *out) {
    *out = {};
    for (int i = 0; i < 3; i++) {
        out->position[i] = mario.engineState.position[i];
        out->velocity[i] = mario.engineState.velocity[i] * SC_FIXED_HZ;
    }
    out->faceAngle = mario.engineState.faceAngle;
    out->forwardSpeed = mario.engineState.forwardVelocity * SC_FIXED_HZ;
    out->floorHeight = mario.engineState.floorHeight;
    out->rawAction = mario.engineState.action;
    out->rawFlags = mario.engineState.flags;
    out->rawActionArg = mario.engineState.actionArg;
    out->rawActionTimer = mario.engineState.actionTimer;
    out->rawAnimation = mario.engineState.animID;
    out->rawAnimationFrame = mario.engineState.animFrame;
    out->attackId = identifyAttack(mario.engineState.action, mario.engineState.actionArg);
    out->interruptedAttackId = identifyAttack(mario.engineState.interruptedAction, 0);
    if (!(mario.engineState.action & MarioAction::ACT_FLAG_AIR)) {
        out->flags |= SC_CAN_INTERACT;
        if (mario.engineState.position[1] <= mario.engineState.floorHeight + 5)
            out->flags |= SC_GROUNDED;
    }
    if ((out->flags & SC_GROUNDED) && equipmentActionEligible(mario.engineState.action))
        out->flags |= SC_CAN_EQUIP_ACTION;
    if (!(mario.engineState.action & MarioAction::ACT_FLAG_INVULNERABLE))
        out->flags |= SC_AUTO_ATTACK_ELIGIBLE;
    if (mario.engineState.action == MarioAction::ACT_IN_CANNON ||
        mario.engineState.action == MarioAction::ACT_SHOT_FROM_CANNON)
        out->flags = 0;
    bool hitting = false;
    switch (out->attackId) {
    case Punch1:
    case Punch2:
        hitting = mario.engineState.flags & MarioAction::MARIO_PUNCHING;
        break;
    case Kick:
    case JumpKick:
        hitting = mario.engineState.flags & MarioAction::MARIO_KICKING;
        break;
    case Dive:
    case SlideKick:
        hitting = true;
        break;
    case Pound:
        hitting = (mario.engineState.action == MarioAction::ACT_GROUND_POUND &&
                   mario.engineState.velocity[1] < 0) ||
                  (mario.engineState.action == MarioAction::ACT_GROUND_POUND_LAND &&
                   mario.engineState.actionTimer <= 2);
        break;
    }
    if (hitting)
        out->flags |= SC_ATTACK_ACTIVE;
    if (mario.engineState.action == MarioAction::ACT_GROUND_POUND_LAND &&
        mario.previousAction != mario.engineState.action)
        out->landingRecoveryTicks = 12;
    mario.previousAction = mario.engineState.action;
}

static bool geometry(SC_Mesh *m, SM64MarioGeometryBuffers &g) {
    if (!m || m->capacityTriangles < SM64_GEO_MAX_TRIANGLES || !m->position || !m->normal ||
        !m->color || !m->uv)
        return false;
    g = {m->position, m->normal, m->color, m->uv, 0};
    return true;
}

static const SC_Descriptor *describe() {
    return &characterProfile;
}

static void clearEquipment(int32_t id) {
    if (adapter.initialized && id >= 0 && id == mario.id && library.setEquipment)
        library.setEquipment(id, 0, 0, 0, 0, 0);
    mario.attachments = {sizeof(SC_Attachments)};
    mario.equipmentPose = {sizeof(SC_EquipmentPose)};
}

static void captureEquipment(int32_t id) {
    mario.attachments = {sizeof(SC_Attachments)};
    mario.attachments.valid =
        library.equipmentHands(id, mario.attachments.right, mario.attachments.left);
    // Native shield bind geometry has its pointed bottom along local +X.
    // Roll that asset frame onto the glove without changing its outward normal.
    if ((mario.attachments.valid & 2) && mario.equipmentPose.leftKind == SC_EQUIPMENT_SHIELD)
        for (int row = 0; row < 3; row++) {
            float x = mario.attachments.left[row];
            mario.attachments.left[row] = -mario.attachments.left[4 + row];
            mario.attachments.left[4 + row] = x;
        }
}

static void shiftEquipment(float x, float y, float z) {
    float delta[3] = {x, y, z};
    for (int axis = 0; axis < 3; axis++) {
        if (mario.attachments.valid & 1)
            mario.attachments.right[12 + axis] += delta[axis];
        if (mario.attachments.valid & 2)
            mario.attachments.left[12 + axis] += delta[axis];
    }
}

static void shutdown() {
    clearEquipment(mario.id);
    if (adapter.initialized) {
        library.setOpenWorld(0);
        if (mario.id >= 0)
            library.deleteMario(mario.id);
        library.terminate();
    }
    adapter.initialized = false;
    mario.id = -1;
    mario.fireAppearance = false;
    mario.engineState = {};
    adapter.ownedRom.clear();
    if (library.module)
        FreeLibrary(library.module);
    library.module = nullptr;
}

static uint32_t initialize(const SC_Init *init, SC_Texture *texture) {
    if (adapter.initialized || !init || init->size != sizeof(*init) || !texture || !texture->rgba ||
        texture->capacityBytes < SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT * 4)
        return 0;
    adapter.log = init->log;
    library.module = LoadLibraryExA(init->runtimePath, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!library.module) {
        if (adapter.log)
            adapter.log("Mario runtime could not be loaded");
        return 0;
    }
    if (!library.bindExports(adapter.log)) {
        shutdown();
        return 0;
    }
    FILE *f = init->assetPath ? fopen(init->assetPath, "rb") : nullptr;
    if (!f) {
        if (adapter.log)
            adapter.log("Owned US Mario 64 ROM is missing");
        shutdown();
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    bool ok = n == 8388608;
    if (ok) {
        adapter.ownedRom.resize(n);
        ok = fread(adapter.ownedRom.data(), 1, n, f) == size_t(n);
    }
    fclose(f);
    if (!ok || adapter.ownedRom[0] != 0x80 || adapter.ownedRom[1] != 0x37 ||
        adapter.ownedRom[2] != 0x12 || adapter.ownedRom[3] != 0x40) {
        if (adapter.log)
            adapter.log("Expected a big-endian US Mario 64 ROM");
        shutdown();
        return 0;
    }
    adapter.cannonAssetsReady = MarioCannonAssets::initialize(adapter.ownedRom, adapter.log);
    library.initialize(adapter.ownedRom.data(), texture->rgba);
    library.initializeAudio(adapter.ownedRom.data());
    library.setBody(characterProfile.body.radiusCm, characterProfile.body.heightCm);
    library.setOpenWorld(1);
    adapter.initialized = true;
    return 1;
}

static int32_t create(float x, float y, float z, float yaw) {
    if (!adapter.initialized || mario.id >= 0)
        return -1;
    mario.id = library.createMario(x, y, z);
    if (mario.id >= 0) {
        mario.fireAppearance = false;
        mario.engineState = {};
        mario.previousAction = 0;
        clearEquipment(mario.id);
        library.applyNativeHurt(mario.id, 0);
        library.setAction(mario.id, MarioAction::ACT_IDLE);
        library.setFacing(mario.id, yaw);
    }
    return mario.id;
}

static void destroy(int32_t id) {
    if (id >= 0 && id == mario.id) {
        clearEquipment(id);
        library.deleteMario(id);
        mario.id = -1;
    }
}

static uint32_t step(int32_t id, const SC_Input *in, SC_State *out, SC_Mesh *mesh) {
    if (id != mario.id || id < 0 || !in || !out)
        return 0;
    SM64MarioGeometryBuffers g;
    if (!geometry(mesh, g))
        return 0;
    SM64MarioInputs input = {in->camLookX, in->camLookZ, -in->moveX, -in->moveY,
                             in->jump,     in->attack,   in->crouch};
    // SM64's cannon uses raw stick yaw, unlike camera-relative locomotion.
    // Positive framework X must turn toward screen-right in the host's +Z view.
    float cannon[7] = {};
    if (library.queryCannon(id, cannon) == SC_LAUNCHER_AIMING)
        input.stickX = in->moveX;
    library.tickMario(id, &input, &mario.engineState, &g);
    mesh->numTrianglesUsed = g.numTrianglesUsed;
    writeFrameworkState(out);
    captureEquipment(id);
    firePresentation(*mesh, true);
    if (mario.engineState.action == MarioAction::ACT_IN_CANNON)
        mesh->numTrianglesUsed = 0;
    return 1;
}

struct Vec {
    float x, y, z;

    Vec operator+(Vec b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Vec operator-(Vec b) const {
        return {x - b.x, y - b.y, z - b.z};
    }

    Vec operator*(float s) const {
        return {x * s, y * s, z * s};
    }
};

static Vec cross(Vec a, Vec b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static Vec unit(Vec v) {
    float n = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return n > .0001f ? v * (1 / n) : Vec{0, 0, 1};
}

// Original procedural accessory, fitted to this character's hand. The shared host
// only knows that this adapter supplies its own item prop.
static void flask(SC_Mesh &mesh) {
    float m[16];
    library.handMatrix(m);
    Vec axis = unit({.09665986f, -.21849427f, -.9710392f});
    Vec u = unit(cross(axis, {1, 0, 0})), v = cross(axis, u), grip = {40, 0, 0};
    auto transform = [&](Vec p) {
        return Vec{p.x * m[0] + p.y * m[4] + p.z * m[8] + m[12],
                   p.x * m[1] + p.y * m[5] + p.z * m[9] + m[13],
                   p.x * m[2] + p.y * m[6] + p.z * m[10] + m[14]};
    };
    const float heights[] = {-125, -110, -50, -15, 0, 70, 75}, radii[] = {0, 45, 42, 30, 15, 15, 0};
    for (int band = 0; band < 6; band++)
        for (int side = 0; side < 10; side++) {
            float a = side * 6.2831853f / 10, b = (side + 1) * 6.2831853f / 10;
            Vec ra = u * cosf(a) + v * sinf(a), rb = u * cosf(b) + v * sinf(b);
            Vec q[4] = {transform(grip + axis * heights[band] + ra * radii[band]),
                        transform(grip + axis * heights[band] + rb * radii[band]),
                        transform(grip + axis * heights[band + 1] + rb * radii[band + 1]),
                        transform(grip + axis * heights[band + 1] + ra * radii[band + 1])};
            for (int half = 0; half < 2; half++) {
                if (mesh.numTrianglesUsed >= mesh.capacityTriangles)
                    return;
                int ids[3] = {0, half ? 2 : 1, half ? 3 : 2};
                Vec n = unit(cross(q[ids[1]] - q[0], q[ids[2]] - q[0]));
                for (int k = 0; k < 3; k++) {
                    size_t i = mesh.numTrianglesUsed * 3 + k;
                    Vec p = q[ids[k]];
                    mesh.position[i * 3] = p.x;
                    mesh.position[i * 3 + 1] = p.y;
                    mesh.position[i * 3 + 2] = p.z;
                    mesh.normal[i * 3] = n.x;
                    mesh.normal[i * 3 + 1] = n.y;
                    mesh.normal[i * 3 + 2] = n.z;
                    mesh.color[i * 3] = 1;
                    mesh.color[i * 3 + 1] = band >= 4 ? .75f : .4f;
                    mesh.color[i * 3 + 2] = .04f;
                    mesh.uv[i * 2] = mesh.uv[i * 2 + 1] = 1;
                }
                ++mesh.numTrianglesUsed;
            }
        }
}

#include "fire_flower.h"

static uint32_t pose(int32_t id, const SC_Pose *p, SC_State *out, SC_Mesh *mesh) {
    if (id != mario.id || id < 0 || !p || !out)
        return 0;
    SM64MarioGeometryBuffers g;
    if (!geometry(mesh, g))
        return 0;
    library.cancelCannon(id);
    if (p->kind != SC_POSE_NATIVE_COMBAT)
        clearEquipment(id);
    int anim = 0xc3, style = 0;
    float phase = -1;
    switch (p->kind) {
    case SC_POSE_CLIMB:
        anim = 5;
        style = 2;
        phase = p->travelCm * .9f;
        break;
    case SC_POSE_DRINK:
        anim = 0x30;
        style = 4;
        phase = p->phase;
        break;
    case SC_POSE_REST:
        anim = 0x84;
        style = 1;
        phase = p->phase;
        break;
    case SC_POSE_DOOR:
        anim = 0x60;
        style = 1;
        phase = p->phase;
        break;
    case SC_POSE_WALK:
        anim = 0x48;
        break;
    case SC_POSE_DOWNED:
        anim = 0x19;
        break;
    case SC_POSE_STAGGER:
        anim = 0x74;
        break;
    case SC_POSE_DEATH:
        anim = 0x32;
        style = 8;
        break;
    }
    library.poseMario(id, p->position[0], p->position[1], p->position[2], p->yaw, anim, phase,
                      style, &mario.engineState, &g);
    mesh->numTrianglesUsed = g.numTrianglesUsed;
    writeFrameworkState(out);
    if (p->kind == SC_POSE_NATIVE_COMBAT)
        captureEquipment(id);
    if (p->kind == SC_POSE_DEATH || p->kind == SC_POSE_NATIVE_COMBAT) {
        out->flags = 0;
        out->attackId = out->interruptedAttackId = -1;
        out->landingRecoveryTicks = 0;
    }
    firePresentation(*mesh, p->kind == SC_POSE_NATIVE_COMBAT);
    if (p->kind == SC_POSE_DRINK && p->phase > .02f)
        flask(*mesh);
    return 1;
}

static void resume(int32_t id, float x, float y, float z, float yaw) {
    if (id == mario.id && id >= 0) {
        library.cancelCannon(id);
        clearEquipment(id);
        library.resumeMario(id, x, y, z, yaw);
        mario.engineState.position[0] = x;
        mario.engineState.position[1] = y;
        mario.engineState.position[2] = z;
        mario.engineState.velocity[0] = mario.engineState.velocity[1] =
            mario.engineState.velocity[2] = mario.engineState.forwardVelocity = 0;
        mario.engineState.faceAngle = yaw;
        mario.previousAction = 0;
    }
}

static void position(int32_t id, float x, float y, float z) {
    if (id == mario.id && id >= 0) {
        library.setPosition(id, x, y, z);
        shiftEquipment(x - mario.engineState.position[0], y - mario.engineState.position[1],
                       z - mario.engineState.position[2]);
        mario.engineState.position[0] = x;
        mario.engineState.position[1] = y;
        mario.engineState.position[2] = z;
    }
}

static void command(int32_t id, uint32_t c) {
    if (id == mario.id && id >= 0)
        library.setAction(id,
                          c == SC_COMMAND_FALL ? MarioAction::ACT_FREEFALL : MarioAction::ACT_IDLE);
}

static void damage(int32_t id, uint32_t severity) {
    if (id == mario.id && id >= 0) {
        if (severity) {
            library.cancelCannon(id);
            clearEquipment(id);
        }
        library.applyNativeHurt(id, std::min(severity, 2u));
    }
}

static void surfaces(const SC_Surface *s, uint32_t n, uint32_t dynamic) {
    if (!adapter.initialized || (!s && n) || n > 2000000 || dynamic && n > 48)
        return;
    std::vector<SM64Surface> dest(n);
    for (uint32_t i = 0; i < n; i++)
        memcpy(dest[i].vertices, s[i].vertices, sizeof(s[i].vertices));
    if (dynamic)
        library.loadHostContacts(dest.data(), n);
    else
        library.loadStaticSurfaces(dest.data(), n);
}

static float floorHeight(float x, float y, float z) {
    return adapter.initialized ? library.findFloorHeight(x, y, z) : -11000;
}

static uint32_t probe(int32_t id, float x, float y, float z) {
    if (id != mario.id || id < 0)
        return 0;
    auto p = library.probeAttack(id, x, y, z);
    return (p & 0x3f ? SC_CONTACT_ATTACK : 0) | (p & 0x40 ? SC_CONTACT_AUTOMATIC : 0);
}

static void contact(int32_t id, float x, float y, float z, float radius) {
    if (id == mario.id && id >= 0)
        library.attackContact(id, x, y, z, radius);
}

static uint32_t audio(uint32_t queued, uint32_t desired, int16_t *stereo, uint32_t capacity) {
    if (!adapter.initialized || !stereo || capacity < 1088)
        return 0;
    return 2 * library.audioTick(queued, desired, stereo);
}

static uint32_t resolveMotion(int32_t id, const SC_MotionResolution *r, SC_State *out, SC_Mesh *m) {
    if (id != mario.id || id < 0 || !r || r->size != sizeof(*r) || r->normalCount > 4 || !out ||
        !m || m->numTrianglesUsed > m->capacityTriangles)
        return 0;
    SCMotion::Point target = {r->position[0], r->position[1], r->position[2]};
    if (!SCMotion::finite(target))
        return 0;
    SCMotion::Point velocity = {mario.engineState.velocity[0], mario.engineState.velocity[1],
                                mario.engineState.velocity[2]},
                    before = velocity;
    for (unsigned i = 0; i < r->normalCount; i++)
        velocity = SCMotion::clip(velocity, {r->normals[i][0], r->normals[i][1], r->normals[i][2]});
    bool horizontal = fabsf(velocity.x - before.x) + fabsf(velocity.z - before.z) > .001f;
    if (horizontal) {
        float forward = velocity.x * sinf(mario.engineState.faceAngle) +
                        velocity.z * cosf(mario.engineState.faceAngle);
        library.setForwardVelocity(id, forward);
        mario.engineState.forwardVelocity = forward;
    }
    // A contact callback can change engine momentum between ticks (a bounce).
    // Empty positional acknowledgements must not overwrite it with old telemetry.
    if (SCMotion::squared(velocity - before) > .000001f)
        library.setVelocity(id, velocity.x, velocity.y, velocity.z);
    library.setPosition(id, target.x, target.y, target.z);
    shiftEquipment(target.x - mario.engineState.position[0],
                   target.y - mario.engineState.position[1],
                   target.z - mario.engineState.position[2]);
    for (unsigned axis = 0; axis < 3; axis++) {
        float delta = r->position[axis] - mario.engineState.position[axis];
        for (unsigned j = 0; j < m->numTrianglesUsed * 3; j++)
            m->position[j * 3 + axis] += delta;
        mario.engineState.position[axis] = r->position[axis];
    }
    mario.engineState.velocity[0] = velocity.x;
    mario.engineState.velocity[1] = velocity.y;
    mario.engineState.velocity[2] = velocity.z;
    for (int k = 0; k < 3; k++) {
        out->position[k] = mario.engineState.position[k];
        out->velocity[k] = mario.engineState.velocity[k] * SC_FIXED_HZ;
    }
    out->forwardSpeed = mario.engineState.forwardVelocity * SC_FIXED_HZ;
    return 1;
}

static uint32_t equipment(int32_t id, const SC_EquipmentPose *p) {
    if (id != mario.id || id < 0 || !p || p->size != sizeof(*p) ||
        (p->rightKind != SC_EQUIPMENT_NONE && p->rightKind != SC_EQUIPMENT_SWORD &&
         (p->rightKind < SC_EQUIPMENT_DAGGER || p->rightKind > SC_EQUIPMENT_PYROMANCY)) ||
        (p->leftKind != SC_EQUIPMENT_NONE && p->leftKind != SC_EQUIPMENT_SHIELD &&
         p->leftKind != SC_EQUIPMENT_PYROMANCY) ||
        p->swing > SC_EQUIPMENT_CAST_LEFT || !std::isfinite(p->swingPhase) ||
        !std::isfinite(p->guard) || p->swingPhase < 0 || p->swingPhase > 1 || p->guard < 0 ||
        p->guard > 1)
        return 0;
    if (!library.setEquipment(id, p->rightKind, p->leftKind, p->swing, p->swingPhase, p->guard))
        return 0;
    if (p->rightKind != mario.equipmentPose.rightKind ||
        p->leftKind != mario.equipmentPose.leftKind || (!p->rightKind && !p->leftKind))
        mario.attachments = {sizeof(SC_Attachments)};
    mario.fireAppearance =
        p->rightKind == SC_EQUIPMENT_PYROMANCY || p->leftKind == SC_EQUIPMENT_PYROMANCY;
    mario.equipmentPose = *p;
    return 1;
}

static uint32_t attachments(int32_t id, SC_Attachments *out) {
    if (id != mario.id || id < 0 || !out || out->size != sizeof(*out))
        return 0;
    *out = mario.attachments;
    return 1;
}

static SC_CharacterAPI api = {sizeof(SC_CharacterAPI),
                              SC_ABI_VERSION,
                              describe,
                              initialize,
                              shutdown,
                              create,
                              destroy,
                              step,
                              pose,
                              resume,
                              position,
                              command,
                              damage,
                              surfaces,
                              floorHeight,
                              probe,
                              contact,
                              audio,
                              resolveMotion,
                              equipment,
                              attachments};

extern "C" __declspec(dllexport) const SC_CharacterAPI *SoulsCharacterGetAPI(uint32_t abi,
                                                                             uint32_t size) {
    return abi == SC_ABI_VERSION && size == sizeof(api) ? &api : nullptr;
}

extern "C" __declspec(dllexport) const SC_CameraProfile *
SoulsCharacterGetCameraProfile(uint32_t version, uint32_t size) {
    return version == SC_CAMERA_PROFILE_VERSION && size == sizeof(SC_CameraProfile)
               ? &characterCameraProfile
               : nullptr;
}

// SMB-inspired low, bouncing fireball. Native host retains casting and damage.
static const SC_ProjectileProfile flowerProjectile = {sizeof(SC_ProjectileProfile),
                                                      SC_PROJECTILE_PROFILE_VERSION,
                                                      SC_PROJECTILE_FIREBALL,
                                                      3.5f,
                                                      1200,
                                                      2400,
                                                      600,
                                                      3,
                                                      75};

extern "C" __declspec(dllexport) const SC_ImpactProfile *
SoulsCharacterGetImpactProfile(uint32_t version, uint32_t size) {
    return version == SC_IMPACT_PROFILE_VERSION && size == sizeof(SC_ImpactProfile)
               ? &characterImpactProfile
               : nullptr;
}

extern "C" __declspec(dllexport) const SC_ProjectileProfile *
SoulsCharacterGetProjectileProfile(uint32_t version, uint32_t size) {
    return version == SC_PROJECTILE_PROFILE_VERSION && size == sizeof(SC_ProjectileProfile)
               ? &flowerProjectile
               : nullptr;
}

static uint32_t launcherBegin(int32_t id, const float *p, float yaw) {
    if (id != mario.id || id < 0 || !adapter.cannonAssetsReady)
        return 0;
    clearEquipment(id);
    return library.beginCannon(id, p, yaw);
}

static void launcherCancel(int32_t id) {
    if (id == mario.id && id >= 0 && adapter.initialized)
        library.cancelCannon(id);
}

static uint32_t launcherQuery(int32_t id, SC_LauncherState *out) {
    if (!out || out->size != sizeof(*out) || id != mario.id || id < 0)
        return 0;
    float data[7] = {};
    out->phase = library.queryCannon(id, data);
    out->ticks = uint32_t(data[6]);
    memcpy(out->position, data, 12);
    out->yaw = data[3];
    out->pitch = data[4];
    out->propLift = data[5];
    return 1;
}

static uint32_t launcherTexture(SC_Texture *out) {
    if (!adapter.cannonAssetsReady || !out || !out->rgba ||
        out->capacityBytes < MarioCannonAssets::atlas.size())
        return 0;
    memcpy(out->rgba, MarioCannonAssets::atlas.data(), MarioCannonAssets::atlas.size());
    return 1;
}

static const SC_LauncherAPI launcherAPI = {sizeof(SC_LauncherAPI),
                                           SC_LAUNCHER_VERSION,
                                           1024,
                                           MarioCannonAssets::width,
                                           MarioCannonAssets::height,
                                           "Cannon",
                                           "Bob-omb Buddy",
                                           launcherBegin,
                                           launcherCancel,
                                           launcherQuery,
                                           launcherTexture,
                                           MarioCannonAssets::mesh};

extern "C" __declspec(dllexport) const SC_LauncherAPI *
SoulsCharacterGetLauncherAPI(uint32_t version, uint32_t size) {
    return version == SC_LAUNCHER_VERSION && size == sizeof(SC_LauncherAPI) ? &launcherAPI
                                                                            : nullptr;
}

#include "../../sdk/class_api.h"
static const SC_ClassProfile startingClass = {
    sizeof(SC_ClassProfile),
    SC_CLASS_FIRECASTER,
    "Mario",
    "Hero from another world.\nJumps, punches and kicks.\nFind a Fire Flower ahead.",
    160.f,
    3.14159265f};

extern "C" __declspec(dllexport) const SC_ClassProfile *
SoulsCharacterGetClassProfile(uint32_t version, uint32_t size) {
    return version == SC_CLASS_PROFILE_VERSION && size == sizeof(startingClass) ? &startingClass
                                                                                : nullptr;
}
