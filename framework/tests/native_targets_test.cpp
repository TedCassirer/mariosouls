#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <unordered_map>
#include "../core/combat.h"
#include "../core/equipment.h"
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
    T v{};
    get(a, v);
    return v;
}

template <class T> static void put(uintptr_t a, T v) {
    for (unsigned n = 0; n < sizeof(T); n++)
        mem[a + n] = ((unsigned char *)&v)[n];
}

#define SC_NATIVE_DAMAGE_MANAGER_ASM "movl _testManager,%edx\n"
#define SC_NATIVE_DAMAGE_ENTRY_ASM "movl $_fakeNative,%edi\n"
#include "../hosts/ds1ptde/native_targets.h"
extern "C" {
__attribute__((used)) volatile uint32_t seenOwner = 0, seenBehavior = 0, seenManager = 0,
                                        seenTarget = 0, seenPoint = 0, testManager = 0x12345678;
}

extern "C" __attribute__((used, naked)) void fakeNative() {
    __asm__ volatile(
        "movl %eax,_seenOwner\n movl %ecx,_seenBehavior\n movl 4(%esp),%edx\n movl %edx,_seenManager\n movl 8(%esp),%edx\n movl %edx,_seenTarget\n movl 12(%esp),%edx\n movl %edx,_seenPoint\n movl $73,%eax\n ret $12\n");
}

int main() {
    put<uint32_t>(0x137d644, 0x100000);
    put<uint32_t>(0x100010, 0x200000);
    put<int>(0x20000c, 0);
    put<uint32_t>(0x100244, 0x300000);
    put<int>(0x300000, 3);
    put<uint32_t>(0x300004, 0x400000);
    const uint32_t a = 0x104001, b = 0x204002;
    put<uint32_t>(0x400020, 0x500000);
    put<uint32_t>(0x500004, a);
    put<int>(0x500208, -1);
    put<uint32_t>(0x400040, 0x600000);
    put<uint32_t>(0x600004, b);
    put<int>(0x600208, -1);
    assert(nativeActor(a) == 0x500000 && nativeActor(b) == 0x600000);
    assert(!nativeActor(0xffffffff) && !nativeActor(a + 3) && !nativeTarget(0x600000, a));
    SC_Descriptor descriptor{};
    descriptor.attackCount = 1;
    descriptor.attacks[0].maxTargets = 8;
    SCCombat::Window window;
    window.observe(0, true, -1);
    assert(window.pay(30, 8, 100));
    assert(window.canHit(int(a), descriptor));
    window.hit.insert(int(a));
    assert(!window.canHit(int(a), descriptor) && window.canHit(int(b), descriptor));
    SCEquipment::Swing swing;
    swing.begin(100);
    swing.age = 5;
    assert(swing.claim(int(a)) && !swing.claim(int(a)) && swing.claim(int(b)));
    put<uint32_t>(0x500004, a + 0x100000);
    assert(!nativeTarget(0x500000, a)); // recycled pointer
    put<uint32_t>(0x500004, a);
    put<uint32_t>(0x400020, 0x600000);
    assert(!nativeActor(a)); // streamed slot
    const unsigned char signature[] = {0x51, 0xd9, 0x05, 0xa4, 0xfb, 0x15, 0x01,
                                       0x53, 0x56, 0x8b, 0x74, 0x24, 0x10};
    for (unsigned i = 0; i < sizeof(signature); i++)
        put<uint8_t>(0xdea8e0 + i, signature[i]);
    assert(nativeDamageEntryReady());
    put<uint8_t>(0xdea8e0, 0);
    assert(!nativeDamageEntryReady());
    // Execute the production thunk with only its two absolute addresses replaced
    // by this test process's fake callee/manager. Register and stack ABI is exact.
    alignas(16) float point[4] = {1, 2, 3, 0};
    for (int i = 0; i < 100; i++) {
        assert(nativeDamageTarget(990050, 0x500000, 0x600000, point) == 73);
        assert(seenOwner == 0x500000 && seenBehavior == 990050 && seenTarget == 0x600000 &&
               seenManager == 0x12345678 && seenPoint == uintptr_t(point));
    }
    puts(
        "PASS native targets: unlabelled enemies, independent hit windows, stale handles, signature refusal and actual x86 dispatch ABI");
}
