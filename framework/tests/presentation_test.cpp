#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>
#include "../sdk/character_api.h"
#include "../core/session.h"
#include "../core/starting_class.h"
#include "../hosts/ds1ptde/interaction.h"
using DWORD = uint32_t;
using std::isfinite;

struct Vec {
    float x, y, z;

    Vec operator+(Vec b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Vec operator-(Vec b) const {
        return {x - b.x, y - b.y, z - b.z};
    }
};

static float dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static uintptr_t player = 0x10000, posData = 0x20000, cam = 0x30000, lastPlayer = 0;
static int area = 1801, character = -1, created = 0, destroyed = 0, poses = 0, captures = 0;
static bool ready = true, presentationActive = false, hideNative = false, poseWorks = true,
            cacheAvailable = false;
static float savedOpacity = 1;
static DWORD timeNow = 1000;
static Vec origin;
static SC_State state;
static SC_Mesh geo{};
static SCSession::Session characterSession;
static SCClass::Selection classSelection;
static SC_Descriptor descriptor{};
static const SC_Descriptor *profile = &descriptor;
static SC_CharacterAPI api{};
static const SC_CharacterAPI *guest = &api;
static const char *ini = "";

static struct {
    bool armed = false;
} characterVisibility;

static std::unordered_map<uintptr_t, std::vector<unsigned char>> memory;

template <class T> static bool get(uintptr_t a, T &v) {
    auto i = memory.find(a);
    if (i == memory.end() || i->second.size() != sizeof(v))
        return false;
    memcpy(&v, i->second.data(), sizeof(v));
    return true;
}

template <class T> static T read(uintptr_t a) {
    T v{};
    get(a, v);
    return v;
}

template <class T> static bool put(uintptr_t a, const T &v) {
    assert(a == player + 0x258);
    auto &b = memory[a];
    b.resize(sizeof(v));
    memcpy(b.data(), &v, sizeof(v));
    return true;
}

template <class T> static void seed(uintptr_t a, const T &v) {
    auto &b = memory[a];
    b.resize(sizeof(v));
    memcpy(b.data(), &v, sizeof(v));
}

static DWORD GetTickCount() {
    return timeNow;
}

static int GetPrivateProfileIntA(const char *, const char *, int, const char *) {
    return 0;
}

static void logf(const char *, ...) {}

static int currentMap() {
    return area;
}

static bool characterOwned() {
    return characterSession.visible();
}

static bool visibilityIdentity(uintptr_t actor) {
    return actor && actor == player && actor == lastPlayer;
}

static void visibilityCapture() {
    characterVisibility.armed = true;
    captures++;
}

static void visibilityRelease() {
    characterVisibility.armed = false;
}

static void classPreviewRelease() {}

static bool loadMap() {
    return cacheAvailable;
}

static void rebuildSurfaces() {}

static HostInteraction::Observation nativeInteractionState() {
    return {};
}

static bool guestPose(int, const SC_Pose *p, SC_State *s, SC_Mesh *m) {
    poses++;
    m->numTrianglesUsed = poseWorks ? 10 : 0;
    s->attackId = 2;
    s->flags = 0xffff;
    return poseWorks;
}

#include "../hosts/ds1ptde/presentation.h"

static void frame() {
    timeNow += 34;
    presentationValidate();
    presentationTick();
}

static void actor(uintptr_t id, int hp) {
    player = id;
    seed(player + 0x28, uint32_t(0x40000));
    seed(player + 0x2d4, hp);
    seed(player + 0x258, 1.f);
    seed(posData + 0x10, Vec{2, 3, 4});
    seed(posData + 4, 0.f);
}

int main() {
    descriptor.capabilities = SC_CAP_DEATH_POSE;
    api.create = [](float, float, float, float) -> int32_t {
        created++;
        return 5;
    };
    api.destroy = [](int32_t) { destroyed++; };
    api.surfaces = [](const SC_Surface *, uint32_t, uint32_t) {};
    actor(0x10000, 500);
    classSelection.observe(player, true, true, area, true);
    characterSession.request(true);
    frame();
    assert(presentationActive && !characterOwned());
    assert(created == 1 && captures == 1);
    assert(read<float>(player + 0x258) == 0 && geo.numTrianglesUsed == 10);
    assert(state.flags == 0 && state.attackId == -1); // no combat authority during presentation
    seed(player + 0x2d4, 0);
    frame();
    assert(presentationActive && created == 1); // death stays guest
    seed(player + 0x2d4, 500);
    frame();
    assert(presentationActive && created == 1); // same-pointer respawn
    seed(posData + 0x10, Vec{60, 30, 4});
    frame();
    assert(origin.x == 60 && presentationActive); // warp needs no guest floor
    area = 1002;
    frame();
    assert(created == 2 && destroyed == 1 && presentationActive);
    player = 0;
    frame();
    assert(!presentationActive && character < 0); // no stale mesh in loading/menu
    actor(0x50000, 500);
    frame();
    assert(presentationActive && created == 3);
    characterSession.request(false);
    frame();
    assert(!presentationActive && read<float>(player + 0x258) == 1); // save switch/manual off
    characterSession.request(true);
    poseWorks = false;
    frame();
    assert(!presentationActive &&
           read<float>(player + 0x258) == 1); // never hide without validated mesh
    int attempts = created;
    frame();
    assert(created == attempts);
    timeNow += 1000;
    poseWorks = true;
    frame();
    assert(presentationActive);
    presentationRelease();
    assert(read<float>(player + 0x258) == 1); // clean handoff to physics activation
    characterSession.activate({player, posData, 0x40000, area});
    attempts = created;
    frame();
    assert(created == attempts && !presentationActive);
    puts(
        "PASS production presentation: class load without floor/cache, death, respawn, warp, zone, actor removal, retry, human restoration and no movement/combat writes");
}
