#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <map>
#include <initializer_list>
#include "../core/body.h"
using std::isfinite;
static std::map<uintptr_t, uint8_t> memory;
static unsigned writes = 0;
static uintptr_t playerSlot = 0x1000, player = 0x4000, lastMap = 0x5000, posData = 0x6000;

template <class T> bool get(uintptr_t address, T &value) {
    auto out = reinterpret_cast<uint8_t *>(&value);
    for (unsigned i = 0; i < sizeof(T); i++) {
        auto found = memory.find(address + i);
        if (found == memory.end())
            return false;
        out[i] = found->second;
    }
    return true;
}

template <class T> T read(uintptr_t address) {
    T value{};
    get(address, value);
    return value;
}

template <class T> void put(uintptr_t address, const T &value) {
    auto in = reinterpret_cast<const uint8_t *>(&value);
    for (unsigned i = 0; i < sizeof(T); i++)
        memory[address + i] = in[i];
    writes++;
}

static uintptr_t chain(uintptr_t address, std::initializer_list<uintptr_t> offsets) {
    for (auto offset : offsets)
        address = read<uintptr_t>(address + offset);
    return address;
}

static void logf(const char *, ...) {}

#include "../hosts/ds1ptde/body.h"

static void setup() {
    memory.clear();
    bodyBinding = {};
    put(playerSlot, uintptr_t(0x2000));
    put(0x2004, uintptr_t(0x3000));
    put(0x3000, player);
    put(player + 0x28, lastMap);
    put(lastMap + 0x1c, posData);
    put(posData + 0x38, uintptr_t(0x7000));
    put(posData + 0x3c, uintptr_t(0x10000));
    put(posData + 0x40, uintptr_t(0x11000));
    put(posData + 0x44, uintptr_t(0x8000));
    put(posData + 0x48, uintptr_t(0x9000));
    put(0x801c, uintptr_t(0x10000));
    put(0x901c, uintptr_t(0x12000));
    put(0x8008, player);
    put(0x9008, player);
    put(posData + 0x204, 1.5f);
    put(posData + 0x208, .4f);
    for (int i = 0; i < 3; i++) {
        uintptr_t shape = 0x10000 + i * 0x1000;
        float radius = i == 0 ? .4f : i == 1 ? .2f : .45f;
        put(shape, uintptr_t(0x110d754));
        put(shape + 0xc, uint32_t(5));
        put(shape + 0x10, radius);
        float points[8] = {0, 1.1f, 0, radius, 0, .4f, 0, radius};
        put(shape + 0x20, points);
    }
    characterBody = {.9f, 22, 105, 24, 110, .05f, .05f, 1};
    writes = 0;
}

int main() {
    setup();
    auto original = memory;
    assert(bodyCapture());
    assert(writes == 0);
    bodyNativeMotion(posData);
    assert(fabsf(read<float>(0x10010) - .22f) < .00001f);
    assert(fabsf(read<float>(0x11010) - .11f) < .00001f);
    assert(fabsf(read<float>(0x12010) - .24f) < .00001f);
    assert(fabsf(read<float>(posData + 0x204) - 1.05f) < .00001f);
    auto modified = memory;
    auto written = writes;
    bodyRelease();
    assert(memory == modified && writes == written);
    assert(bodyCapture());
    assert(memory == modified && writes == written); // Reactivation before deferred restore.
    bodyNativeMotion(posData);
    bodyRelease();
    bodyNativeMotion(posData);
    assert(memory == original);
    assert(!bodyBinding.actor);
    assert(bodyCapture());
    bodyNativeMotion(posData);
    bodyRelease();
    bodyNativeMotion(posData);
    assert(memory == original);
    setup();
    assert(bodyCapture());
    bodyNativeMotion(posData);
    put(0x3000, uintptr_t(0x14000));
    written = writes;
    bodyRelease();
    bodyNativeMotion(posData);
    assert(writes == written &&
           !bodyBinding.actor); // Never write to a streamed-out/replaced actor.
    setup();
    put(0x1200c, uint32_t(99));
    written = writes;
    assert(!bodyCapture());
    assert(writes == written);
    setup();
    characterBody = {1, 0, 0, 0, 0, 1, 1, 0};
    assert(bodyCapture());
    assert(writes == 0 && !bodyBinding.actor);
    puts(
        "PASS native body: physics-thread writes, three capsules, deferred restore, rapid reactivation, stale-actor rejection, invalid shape, original-size opt-out");
}
