#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "memory.h"

namespace HostBindings {
using Log = void (*)(const char *, ...);

// Accept an address only when its signature is unique in executable sections.
// A mismatched executable therefore leaves the binding unavailable.
static uintptr_t scan(const char *sig, int immediate, Log report) {
    std::vector<int> pat;
    const char *s = sig;
    while (*s) {
        if (*s == ' ') {
            s++;
            continue;
        }
        if (*s == '?') {
            pat.push_back(-1);
            s++;
        } else {
            char *e;
            pat.push_back(strtoul(s, &e, 16));
            s = e;
        }
    }
    auto base = (BYTE *)GetModuleHandleA(NULL);
    auto dos = (IMAGE_DOS_HEADER *)base;
    auto nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    uintptr_t hit = 0;
    int count = 0;
    for (unsigned j = 0; j < nt->FileHeader.NumberOfSections; j++)
        if (sec[j].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            BYTE *b = base + sec[j].VirtualAddress;
            size_t len = sec[j].Misc.VirtualSize;
            for (size_t i = 0; i + pat.size() <= len; i++) {
                bool ok = true;
                for (size_t k = 0; k < pat.size(); k++)
                    if (pat[k] >= 0 && b[i + k] != pat[k]) {
                        ok = false;
                        break;
                    }
                if (ok) {
                    hit = (uintptr_t)(b + i);
                    count++;
                }
            }
        }
    report("signature matches=%d at=%08x", count, (unsigned)hit);
    return count == 1 ? SCNativeMemory::read<uint32_t>(hit + immediate) : 0;
}

// These are addresses of executable-owned slots, not the actors/cameras they
// point to. They survive loads; transient pointees must be re-read and validated.
struct Slots {
    uintptr_t player = 0, camera = 0, map = 0, eventFlags = 0;

    bool discover(Log report) {
        player = scan("8B 15 ? ? ? ? F3 0F 10 44 24 30 52", 2, report);
        camera = scan("D9 45 08 A1 ? ? ? ? 51 D9 1C 24 50", 4, report);
        if (!player || !camera)
            return false;
        map = scan(
            "8B 48 04 8B 11 50 8B 42 34 FF D0 C7 46 2C 00 00 00 00 C3 8B 0D ? ? ? ? 8B 89 50 0B 00 00 32 C0 83 79 28 00",
            0x15, report);
        if (!map)
            return false;
        eventFlags = scan("56 8B F1 8B 46 1C 50 A1 ? ? ? ? 32 C9", 8, report);
        return true;
    }
};
} // namespace HostBindings
