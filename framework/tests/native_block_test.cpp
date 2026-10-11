#undef NDEBUG
// Tests native packet observations with fake memory only; never opens the game.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <initializer_list>
#include <unordered_map>
#include <cassert>
static std::unordered_map<uintptr_t, unsigned char> mem;

template <class T> static bool get(uintptr_t a, T &v) {
    for (unsigned n = 0; n < sizeof(T); n++) {
        auto i = mem.find(a + n);
        if (i == mem.end())
            return false;
        ((unsigned char *)&v)[n] = i->second;
    }
    return true;
}

template <class T> static T read(uintptr_t a) {
    T t{};
    get(a, t);
    return t;
}

template <class T> static void put(uintptr_t a, T v) {
    for (unsigned n = 0; n < sizeof(T); n++)
        mem[a + n] = ((unsigned char *)&v)[n];
}

static uintptr_t chain(uintptr_t p, std::initializer_list<unsigned> offsets) {
    for (auto o : offsets) {
        if (!p)
            return 0;
        p = read<uint32_t>(p + o);
    }
    return p;
}

static BOOL WINAPI fakeMemory(HANDLE, LPCVOID a, LPVOID out, SIZE_T size, SIZE_T *got) {
    *got = 0;
    for (SIZE_T n = 0; n < size; n++) {
        auto i = mem.find(uintptr_t(a) + n);
        if (i == mem.end())
            return FALSE;
        ((unsigned char *)out)[n] = i->second;
        ++*got;
    }
    return TRUE;
}

#define ReadProcessMemory fakeMemory
static uintptr_t player = 0x100000, lastPlayer = 0x100000;
static bool owned = true;

static bool characterOwned() {
    return owned;
}

static bool nativeActionOwned() {
    return false;
}

static void logf(const char *, ...) {}

struct Vec {
    float x, y, z;
};

static int (*nativeDamage)(int, int, int, float, float, float) = nullptr;

static bool nativeTarget(uintptr_t, uint32_t) {
    return false;
}

static int nativeDamageTarget(int, uintptr_t, uintptr_t, const float *) {
    return -1;
}

#include "../hosts/ds1ptde/equipment_native.h"

static void packet(int hp, int damage, int drain, int block = 1, int broken = 0, int br = 0) {
    put<int>(player + 0x2d4, hp);
    put<int>(player + 0x2d8, 1000);
    put<unsigned char>(0x200104, block);
    put<unsigned char>(0x200154, broken);
    put<unsigned char>(0x200155, br);
    put<int>(0x20016c, damage);
    put<int>(0x200170, drain);
}

int main() {
    // Native directories can append an unsorted family. Cache entries must follow
    // current offsets/indices and reject wrong types, missing IDs and duplicates.
    put<uint32_t>(0x13785b4, 0x300000);
    put<int>(0x300000 + 13 * 36 + 8, 1);
    put<uint32_t>(0x300000 + 13 * 36 + 12, 0x400000);
    put<uint32_t>(0x400020, 0x600000);
    for (int i = 0; i < 48; i++)
        mem[0x600000 + i] = 0;
    put<uint16_t>(0x60000a, 4);
    const char name[] = "BEHAVIOR_PARAM_ST";
    for (unsigned i = 0; i < sizeof(name); i++)
        mem[0x60000c + i] = name[i];
    int ids[] = {10, 30, 20, 40};
    for (int i = 0; i < 4; i++) {
        put<int>(0x600030 + i * 12, ids[i]);
        put<int>(0x600034 + i * 12, 0x100 + i * 32);
        put<int>(0x600038 + i * 12, 0);
    }
    assert(equipmentParamRow(13, name, 20) == 0x600140);
    assert(!equipmentParamRow(13, name, 99));
    assert(!equipmentParamRow(13, "OTHER", 20));
    put<int>(0x600034 + 2 * 12, 0x200);
    assert(equipmentParamRow(13, name, 20) == 0x600200);
    put<int>(0x600030 + 2 * 12, 25);
    put<int>(0x600030 + 3 * 12, 20);
    assert(equipmentParamRow(13, name, 20) == 0x600160);
    put<int>(0x600034 + 3 * 12, 0x20);
    assert(!equipmentParamRow(13, name, 20));
    put<int>(0x600030 + 2 * 12, 30);
    assert(!equipmentParamRow(13, name, 99));
    puts(
        "PASS native parameters: unordered families, exact IDs/types, changed offsets/indices, malformed bounds, duplicate rejection");
    {
        unsigned char b[] = {0xc6, 0x83, 0x4, 0x1, 0x0, 0x0, 0x1};
        for (unsigned i = 0; i < sizeof(b); i++)
            mem[0xe7f4b0 + i] = b[i];
    }
    {
        unsigned char b[] = {0x8b, 0x86, 0xe4, 0x2,  0x0,  0x0,  0x3b, 0x83, 0x70, 0x1, 0x0, 0x0,
                             0x7f, 0x28, 0x8b, 0x43, 0x1c, 0xc6, 0x83, 0x54, 0x1,  0x0, 0x0, 0x1};
        for (unsigned i = 0; i < sizeof(b); i++)
            mem[0xe7f510 + i] = b[i];
    }
    {
        unsigned char b[] = {0x8a, 0x8b, 0xc5, 0x0, 0x0,  0x0,  0x8b, 0xbb, 0x6c, 0x1,  0x0,
                             0x0,  0x80, 0xe1, 0x1, 0xf7, 0xdf, 0xe8, 0x28, 0xe3, 0xff, 0xff};
        for (unsigned i = 0; i < sizeof(b); i++)
            mem[0xe6c052 + i] = b[i];
    }
    {
        unsigned char b[] = {0x8b, 0x8e, 0xe4, 0x2,  0x0,  0x0,  0x2b, 0xc8,
                             0x51, 0x8b, 0xd6, 0xe8, 0x7a, 0xb7, 0xff, 0xff};
        for (unsigned i = 0; i < sizeof(b); i++)
            mem[0xe6c1d6 + i] = b[i];
    }

    assert(equipmentBlockPacketLayout());
    // A native outgoing attack has neither incoming HP loss nor a block result.
    // An unrelated stamina caller must never create a block or guard-break event.
    packet(464, 0, 0);
    put<int>(player + 0x2e4, 80);
    equipmentObserveNativeBlock(player, 93, 80, 0xfe024f, 0x200000);
    assert(!equipmentConsumeNativeBlock(player, 464, 464));
    assert(!equipmentConsumeNativeGuardBreak(player));
    packet(431, 33, 32);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    assert(equipmentConsumeNativeBlock(player, 464, 431));
    assert(!equipmentConsumeNativeBlock(player, 464, 431));
    packet(431, 33, 32);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    assert(!equipmentConsumeNativeBlock(player, 529, 431));
    packet(431, 33, 32, 0);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    assert(!equipmentConsumeNativeBlock(player, 464, 431));
    packet(431, 33, 32);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    equipmentBlockImpact.observed -= 101;
    assert(!equipmentConsumeNativeBlock(player, 464, 431));
    packet(431, 33, 32);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    packet(398, 33, 32);
    equipmentObserveNativeBlock(player, 61, 29, 0xe6c1e6, 0x200000);
    assert(equipmentConsumeNativeBlock(player, 464, 398));
    packet(398, 0, 100, 1, 1);
    put<int>(player + 0x2e4, -7);
    equipmentObserveNativeBlock(player, 93, -7, 0xe6c1e6, 0x200000);
    assert(!equipmentConsumeNativeBlock(player, 431, 398));
    assert(equipmentConsumeNativeGuardBreak(player));
    assert(!equipmentConsumeNativeGuardBreak(player));
    packet(398, 0, 100, 1, 1);
    equipmentObserveNativeBlock(player, 93, -7, 0xe6c1e6, 0x200000);
    equipmentClearNativeBlock();
    assert(!equipmentConsumeNativeGuardBreak(player));
    packet(431, 33, 32);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    owned = false;
    assert(!equipmentConsumeNativeBlock(player, 464, 431));
    owned = true;
    // Malformed packet data, another actor, and a changed stamina pool are not evidence.
    packet(431, 33, 31);
    equipmentObserveNativeBlock(player, 93, 61, 0xe6c1e6, 0x200000);
    assert(!equipmentConsumeNativeBlock(player, 464, 431));
    packet(431, 33, 32);
    equipmentObserveNativeBlock(player + 4, 93, 61, 0xe6c1e6, 0x200000);
    assert(!equipmentConsumeNativeBlock(player, 464, 431));
    packet(398, 0, 100, 1, 1);
    put<int>(player + 0x2e4, -6);
    equipmentObserveNativeBlock(player, 93, -7, 0xe6c1e6, 0x200000);
    assert(!equipmentConsumeNativeGuardBreak(player));
    // The observer is read-only: recorded native HP and stamina never change.
    assert(read<int>(player + 0x2d4) == 398 && read<int>(player + 0x2e4) == -6);
    puts(
        "PASS: exact block interval, one-shot, rear/unmatched/expired rejection, adjacent block chain, zero-chip native guard break, reset and ownership release");
}
