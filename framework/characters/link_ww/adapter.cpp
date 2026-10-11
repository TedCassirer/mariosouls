// Wind Waker assets with an authored controller, not an embedded WW runtime.
#include "character_api.h"
#include "profile.generated.h"
#include "../../core/mesh_pack.h"
#include "../../core/kinematic.h"
#include <cmath>
#include <cstring>

namespace {
SCMesh::Pack pack;
SCKinematic::World world;
SC_State state{};
bool ready = false, alive = false, lastJump = false, lastAttack = false, lastRoll = false,
     queued = false;

enum Action { LOCOMOTION, SLASH, ROLL, HURT, LAND };

Action action = LOCOMOTION;
unsigned timer = 0, combatTicks = 0, poseTicks = 0;
int combo = 0, currentClip = -1;
float frame = 0;
uint32_t previousPose = SC_POSE_NEUTRAL;
float previousPhase = 0;
uint32_t groups = 1 | 2 | 4 | 128;
std::vector<float> matrices, blendFrom;
unsigned blendTicks = 0;

const SC_Descriptor *describe() {
    return &characterProfile;
}

bool valid(int id) {
    return id == 0 && alive;
}

void choose(const char *name, float at, bool smooth = true) {
    int clip = pack.clip(name);
    if (clip < 0)
        clip = pack.clip("wait");
    if (clip != currentClip) {
        blendFrom = matrices;
        blendTicks = smooth ? 4 : 0;
        currentClip = clip;
    }
    frame = at;
    pack.sample(currentClip, frame, matrices);
    if (blendTicks && blendFrom.size() == matrices.size()) {
        float t = (5 - blendTicks) / 4.f;
        for (size_t i = 0; i < matrices.size(); i++)
            matrices[i] = blendFrom[i] * (1 - t) + matrices[i] * t;
        blendTicks--;
    }
    state.rawAnimation = currentClip;
    state.rawAnimationFrame = int(frame);
}

uint32_t mesh(SC_State *s, SC_Mesh *m) {
    if (!s || !pack.draw(matrices, groups, state.position, state.faceAngle, m))
        return 0;
    *s = state;
    return 1;
}

void reset(float x, float y, float z, float yaw) {
    state = {};
    state.position[0] = x;
    state.position[1] = y;
    state.position[2] = z;
    state.faceAngle = yaw;
    state.attackId = state.interruptedAttackId = -1;
    state.floorHeight = world.floor(x, y + 25, z);
    if (y <= state.floorHeight + .5f)
        state.flags = SC_GROUNDED | SC_CAN_INTERACT;
    action = LOCOMOTION;
    timer = combatTicks = poseTicks = 0;
    previousPose = SC_POSE_NEUTRAL;
    previousPhase = 0;
    combo = 0;
    lastJump = lastAttack = lastRoll = queued = false;
    currentClip = -1;
    blendTicks = 0;
    groups = 1 | 2 | 4 | 128;
    choose("wait", 0, false);
}

uint32_t initialize(const SC_Init *i, SC_Texture *t) {
    if (ready || !i || i->size != sizeof(*i) || !t || !t->rgba || !pack.load(i->assetPath))
        return 0;
    if (pack.width != characterProfile.textureWidth ||
        pack.height != characterProfile.textureHeight ||
        pack.vertices.size() / 3 > characterProfile.maxTriangles ||
        t->capacityBytes < pack.texture.size()) {
        pack.clear();
        return 0;
    }
    const char *required[] = {
        "wait",        "waits",         "walk",          "dash",       "walks",
        "dashs",       "rollf",         "cuta",          "cutl",       "cutr",
        "cutturn",     "jmpst",         "mjmp",          "jmped",      "rest",
        "koshikake",   "crouch",        "ladderltor",    "ladderrtol", "bindrinkpre",
        "bindrinking", "bindrinkafter", "dooropenalink", "talka",      "damf"};
    for (auto name : required)
        if (pack.clip(name) < 0) {
            pack.clear();
            return 0;
        }
    memcpy(t->rgba, pack.texture.data(), pack.texture.size());
    ready = true;
    if (i->log)
        i->log("Wind Waker Link: owned assets loaded; authored controller / native host gameplay");
    return 1;
}

void shutdown() {
    alive = ready = false;
    pack.clear();
    world.clear();
    matrices.clear();
    blendFrom.clear();
}

int32_t create(float x, float y, float z, float yaw) {
    if (!ready || alive)
        return -1;
    alive = true;
    reset(x, y, z, yaw);
    return 0;
}

void destroy(int32_t id) {
    if (id == 0)
        alive = false;
}

void startAttack(int index) {
    action = SLASH;
    combo = index;
    timer = 0;
    queued = false;
    combatTicks = 120;
    state.attackId = index;
}

uint32_t step(int32_t id, const SC_Input *i, SC_State *s, SC_Mesh *m) {
    if (!valid(id) || !i || !s || !m || m->capacityTriangles < characterProfile.maxTriangles ||
        !std::isfinite(i->moveX) || !std::isfinite(i->moveY) || !std::isfinite(i->camLookX) ||
        !std::isfinite(i->camLookZ))
        return 0;
    bool jump = i->jump && !lastJump, attack = i->attack && !lastAttack,
         roll = i->crouch && !lastRoll;
    lastJump = i->jump;
    lastAttack = i->attack;
    lastRoll = i->crouch;
    bool grounded = (state.flags & SC_GROUNDED) != 0;
    state.flags = grounded ? SC_GROUNDED : 0;
    state.interruptedAttackId = -1;
    state.landingRecoveryTicks = 0;
    if (combatTicks)
        combatTicks--;
    float mx = std::clamp(i->moveX, -1.f, 1.f), my = std::clamp(i->moveY, -1.f, 1.f),
          length = sqrtf(mx * mx + my * my);
    if (length > 1) {
        mx /= length;
        my /= length;
        length = 1;
    }
    float cx = i->camLookX, cz = i->camLookZ, cl = sqrtf(cx * cx + cz * cz);
    if (cl < .01f) {
        cx = 0;
        cz = 1;
    } else {
        cx /= cl;
        cz /= cl;
    }
    float dx = cz * mx + cx * my, dz = -cx * mx + cz * my;
    if (action == LOCOMOTION) {
        if (length > .1f)
            state.faceAngle = atan2f(dx, dz);
        if (attack && grounded)
            startAttack(i->crouch ? 3 : 0);
        else if (roll && grounded && length > .1f) {
            action = ROLL;
            timer = 0;
        } else if (jump && grounded) {
            state.velocity[1] = 390;
            grounded = false;
            state.flags = 0;
            timer = 0;
        }
    } else if (action == SLASH && attack && timer >= 3 && combo < 2)
        queued = true;
    float speed = length * 330;
    if (action == SLASH) {
        speed = timer < 9 ? 60.f : 0;
        dx = sinf(state.faceAngle);
        dz = cosf(state.faceAngle);
    } else if (action == ROLL) {
        speed = timer < 15 ? 470.f : 180.f;
        dx = sinf(state.faceAngle);
        dz = cosf(state.faceAngle);
    } else if (action == HURT || action == LAND) {
        speed = 0;
    }
    state.velocity[0] = dx * speed;
    state.velocity[2] = dz * speed;
    if (!grounded)
        state.velocity[1] = std::max(-1200.f, state.velocity[1] - 900.f / 30);
    float oldY = state.position[1];
    world.horizontal(state.position, state.velocity[0] / 30, state.velocity[2] / 30,
                     characterProfile.body.radiusCm, characterProfile.body.heightCm);
    state.position[1] += state.velocity[1] / 30;
    float floor =
        world.floor(state.position[0], std::max(oldY, state.position[1]) + (grounded ? 25.f : 1.f),
                    state.position[2]);
    float ceiling = world.ceiling(state.position[0], oldY + characterProfile.body.heightCm - 1,
                                  state.position[2]);
    if (state.velocity[1] > 0 && state.position[1] + characterProfile.body.heightCm > ceiling) {
        state.position[1] = std::max(oldY, ceiling - characterProfile.body.heightCm);
        state.velocity[1] = 0;
    }
    bool landed = false;
    if (state.velocity[1] <= 0 && state.position[1] <= floor + (grounded ? 25.f : 0.f)) {
        // Small native-floor corrections and triangle seams are not jump landings.
        landed = !grounded && state.velocity[1] < -90;
        state.position[1] = floor;
        state.velocity[1] = 0;
        grounded = true;
    } else
        grounded = false;
    state.floorHeight = floor;
    state.flags = grounded ? SC_GROUNDED : 0;
    if (landed && action == LOCOMOTION) {
        action = LAND;
        timer = 0;
    }
    groups = combatTicks ? (1 | 8 | 16 | 32 | 64) : (1 | 2 | 4 | 128);
    if (action == SLASH) {
        const char *clips[] = {"cuta", "cutl", "cutr", "cutturn"};
        choose(clips[combo], float(timer));
        state.attackId = combo;
        if (timer >= 4 && timer <= (combo == 3 ? 16 : 10))
            state.flags |= SC_ATTACK_ACTIVE;
        unsigned end = combo == 3 ? 35 : 21;
        if (++timer >= end) {
            if (queued && combo < 2)
                startAttack(combo + 1);
            else {
                action = LOCOMOTION;
                timer = 0;
                state.attackId = -1;
            }
        }
    } else {
        state.attackId = -1;
        if (action == ROLL) {
            choose("rollf", float(timer));
            if (++timer >= 23) {
                action = LOCOMOTION;
                timer = 0;
            }
        } else if (action == HURT) {
            choose("damf", float(timer));
            if (++timer >= 22) {
                action = LOCOMOTION;
                timer = 0;
            }
        } else if (action == LAND) {
            choose("jmped", float(timer));
            if (++timer >= 6) {
                action = LOCOMOTION;
                timer = 0;
            }
        } else if (!grounded) {
            choose("mjmp", std::min(12.f, float(timer++)));
        } else {
            const char *clip = length < .1f ? (combatTicks ? "waits" : "wait")
                                            : (length < .45f ? (combatTicks ? "walks" : "walk")
                                                             : (combatTicks ? "dashs" : "dash"));
            choose(clip, currentClip == pack.clip(clip)
                             ? frame + (length < .1f ? 1.f : std::max(.3f, length) * 1.15f)
                             : 0);
        }
    }
    if (grounded && action == LOCOMOTION)
        state.flags |= SC_CAN_INTERACT;
    state.forwardSpeed = speed;
    state.rawAction = action;
    state.rawActionTimer = timer;
    return mesh(s, m);
}

uint32_t pose(int32_t id, const SC_Pose *p, SC_State *s, SC_Mesh *m) {
    if (!valid(id) || !p || !s || !m || !std::isfinite(p->yaw) || !std::isfinite(p->phase) ||
        !std::isfinite(p->travelCm))
        return 0;
    for (float x : p->position)
        if (!std::isfinite(x))
            return 0;
    memcpy(state.position, p->position, 12);
    state.faceAngle = p->yaw;
    memset(state.velocity, 0, 12);
    state.attackId = state.interruptedAttackId = -1;
    state.flags = SC_GROUNDED;
    state.forwardSpeed = 0;
    state.landingRecoveryTicks = 0;
    groups = 1 | 2 | 4 | 128;
    float phase = std::clamp(p->phase, 0.f, 1.f);
    bool leaving = p->kind == previousPose && phase < previousPhase - .002f;
    if (p->kind != previousPose)
        poseTicks = 0;
    poseTicks++;
    auto duration = [](const char *name) { return float(pack.clips[pack.clip(name)].frames - 1); };
    switch (p->kind) {
    case SC_POSE_CLIMB: {
        float cycle = fmodf(fabsf(p->travelCm) / 55.f, 2.f);
        choose(cycle < 1 ? "ladderltor" : "ladderrtol", fmodf(cycle, 1.f) * 9, false);
        break;
    }
    case SC_POSE_DRINK:
        groups = 1 | 4 | 128 | 256 | 512;
        if (leaving)
            choose("bindrinkafter", (1 - phase) * duration("bindrinkafter"), false);
        else
            choose(phase < .99f ? "bindrinkpre" : "bindrinking",
                   phase < .99f ? phase * duration("bindrinkpre") : float(poseTicks), false);
        break;
    case SC_POSE_REST: {
        choose("crouch", float(poseTicks), false);
        std::vector<float> standing;
        pack.sample(pack.clip("wait"), float(poseTicks), standing);
        for (size_t i = 0; i < matrices.size(); i++)
            matrices[i] = standing[i] + (matrices[i] - standing[i]) * phase;
        break;
    }
    case SC_POSE_DOOR:
        choose("dooropenalink", phase * 79, false);
        break;
    case SC_POSE_WALK:
        choose("walk", float(poseTicks));
        break;
    case SC_POSE_DOWNED:
        choose("damf", 21, false);
        break;
    case SC_POSE_TALK:
        choose("talka", float(poseTicks));
        break;
    default:
        choose("wait", float(poseTicks));
        break;
    }
    previousPose = p->kind;
    previousPhase = phase;
    state.rawAction = 100 + p->kind;
    return mesh(s, m);
}

void position(int32_t id, float x, float y, float z) {
    if (valid(id) && std::isfinite(x) && std::isfinite(y) && std::isfinite(z)) {
        state.position[0] = x;
        state.position[1] = y;
        state.position[2] = z;
    }
}

void resume(int32_t id, float x, float y, float z, float yaw) {
    if (valid(id))
        reset(x, y, z, yaw);
}

void command(int32_t id, uint32_t c) {
    if (!valid(id))
        return;
    if (c == SC_COMMAND_FALL) {
        state.flags = 0;
        state.velocity[1] = std::min(0.f, state.velocity[1]);
    } else {
        action = LOCOMOTION;
        timer = 0;
        state.attackId = -1;
    }
}

void damage(int32_t id, uint32_t severity) {
    if (valid(id) && severity) {
        state.interruptedAttackId = state.attackId;
        state.attackId = -1;
        state.flags &= ~SC_ATTACK_ACTIVE;
        action = HURT;
        timer = 0;
        queued = false;
    }
}

void surfaces(const SC_Surface *s, uint32_t count, uint32_t dynamic) {
    world.surfaces(s, count, dynamic != 0);
}

float floorHeight(float x, float y, float z) {
    return world.floor(x, y, z);
}

uint32_t probe(int32_t id, float x, float y, float z) {
    if (!valid(id) || state.attackId < 0 || !(state.flags & SC_ATTACK_ACTIVE) ||
        !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        return 0;
    const auto &a = characterProfile.attacks[state.attackId];
    float dx = x - state.position[0], dy = y - state.position[1], dz = z - state.position[2];
    if (dy < a.minHeightCm || dy > a.maxHeightCm || dx * dx + dz * dz > a.rangeCm * a.rangeCm)
        return 0;
    if (!(a.flags & SC_ATTACK_RADIAL) &&
        dx * sinf(state.faceAngle) + dz * cosf(state.faceAngle) < -10)
        return 0;
    return SC_CONTACT_ATTACK;
}

void contact(int32_t, float, float, float, float) {}

uint32_t audio(uint32_t, uint32_t, int16_t *, uint32_t) {
    return 0;
}

uint32_t resolveMotion(int32_t id, const SC_MotionResolution *r, SC_State *s, SC_Mesh *m) {
    if (!valid(id) || !r || r->size != sizeof(*r) || r->normalCount > 4)
        return 0;
    for (float v : r->position)
        if (!std::isfinite(v))
            return 0;
    for (unsigned j = 0; j < r->normalCount; j++) {
        float length = 0;
        for (float n : r->normals[j]) {
            if (!std::isfinite(n))
                return 0;
            length += n * n;
        }
        if (fabsf(length - 1) > .05f)
            return 0;
    }
    memcpy(state.position, r->position, 12);
    for (unsigned j = 0; j < r->normalCount; j++) {
        float into = 0;
        for (int k = 0; k < 3; k++)
            into += state.velocity[k] * r->normals[j][k];
        if (into < 0)
            for (int k = 0; k < 3; k++)
                state.velocity[k] -= into * r->normals[j][k];
    }
    return mesh(s, m);
}

SC_CharacterAPI api = {sizeof(SC_CharacterAPI),
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
                       resolveMotion};
} // namespace

extern "C" __declspec(dllexport) const SC_CharacterAPI *SoulsCharacterGetAPI(uint32_t abi,
                                                                             uint32_t size) {
    return abi == SC_ABI_VERSION && size == sizeof(api) ? &api : nullptr;
}
