#pragma once

// Chr handles identify ordinary streamed enemies too. EMEVD entity IDs (+208)
// are optional script labels, not actor identities (many enemies use -1).
static uintptr_t nativeActor(uint32_t handle) {
    if (handle == 0xffffffffu)
        return 0;
    uintptr_t manager = read<uint32_t>(0x137d644);
    if (!manager)
        return 0;
    uintptr_t limits = read<uint32_t>(manager + 0x10);
    if (!limits)
        return 0;
    int groups = read<int>(limits + 0xc) + 2;
    unsigned group = (handle >> 14) & 63, index = handle & 0x3fff;
    if (groups < 2 || groups > 64 || group >= unsigned(groups))
        return 0;
    uintptr_t table = read<uint32_t>(manager + 0x240 + group * 4);
    if (!table)
        return 0;
    int count = read<int>(table);
    uintptr_t entries = read<uint32_t>(table + 4);
    if (!entries || count < 1 || count > 16384 || index >= unsigned(count))
        return 0;
    uintptr_t actor = read<uint32_t>(entries + index * 32);
    uint32_t observed = 0;
    return actor && get(actor + 4, observed) && observed == handle ? actor : 0;
}

static bool nativeTarget(uintptr_t actor, uint32_t handle) {
    return actor && nativeActor(handle) == actor;
}

static bool nativeDamageEntryReady() {
    const unsigned char expected[] = {0x51, 0xd9, 0x05, 0xa4, 0xfb, 0x15, 0x01,
                                      0x53, 0x56, 0x8b, 0x74, 0x24, 0x10};
    unsigned char actual[sizeof(expected)] = {};
    return get(uintptr_t(0xdea8e0), actual) && !memcmp(actual, expected, sizeof(expected));
}

// D78660 uses this same builder after resolving its two script IDs. Pass actual
// native actors instead: EAX=source, ECX=behavior; stack=manager,target,point.
// The builder records target+4 and retains native defenses, death and rewards.
#ifndef SC_NATIVE_DAMAGE_MANAGER_ASM
#define SC_NATIVE_DAMAGE_MANAGER_ASM "movl 0x137d6ac,%edx\n"
#define SC_NATIVE_DAMAGE_ENTRY_ASM "movl $0xdea8e0,%edi\n"
#endif
__attribute__((naked)) static int __cdecl nativeDamageTarget(int behavior, uintptr_t owner,
                                                             uintptr_t target, const float *point) {
    __asm__ volatile(
        "pushl %ebx\n pushl %esi\n pushl %edi\n movl 16(%esp),%ecx\n movl 20(%esp),%eax\n movl 24(%esp),%edx\n movl 28(%esp),%ebx\n pushl %ebx\n pushl %edx\n" SC_NATIVE_DAMAGE_MANAGER_ASM
        "pushl %edx\n" SC_NATIVE_DAMAGE_ENTRY_ASM
        "call *%edi\n popl %edi\n popl %esi\n popl %ebx\n ret\n");
}

#undef SC_NATIVE_DAMAGE_MANAGER_ASM
#undef SC_NATIVE_DAMAGE_ENTRY_ASM
