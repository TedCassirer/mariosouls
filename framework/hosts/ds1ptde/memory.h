#pragma once
// Native memory access stays inside the host. Failed reads return a zero value;
// callers that must distinguish zero from an unavailable actor use get().
#include <windows.h>
#include <stdint.h>
#include <initializer_list>

namespace SCNativeMemory {
template <class T> static bool get(uintptr_t a, T &v) {
    SIZE_T n = 0;
    return a > 0x10000 && ReadProcessMemory(GetCurrentProcess(), (void *)a, &v, sizeof(v), &n) &&
           n == sizeof(v);
}

template <class T> static T read(uintptr_t a) {
    T v = {};
    get(a, v);
    return v;
}

template <class T> static bool put(uintptr_t a, const T &v) {
    SIZE_T n = 0;
    return a > 0x10000 && WriteProcessMemory(GetCurrentProcess(), (void *)a, &v, sizeof(v), &n) &&
           n == sizeof(v);
}

static uintptr_t chain(uintptr_t p, std::initializer_list<unsigned> offsets) {
    for (auto o : offsets) {
        if (!p)
            return 0;
        p = read<uint32_t>(p + o);
    }
    return p;
}

} // namespace SCNativeMemory
