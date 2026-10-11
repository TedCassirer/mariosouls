#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

// The production scanner reads this local synthetic PE image. No game process,
// installed executable or native hook is involved.
static std::vector<unsigned char> image(2048);

static HMODULE fakeModule(LPCSTR) {
    return reinterpret_cast<HMODULE>(image.data());
}

#define GetModuleHandleA fakeModule
#include "../hosts/ds1ptde/native_bindings.h"
#undef GetModuleHandleA

static void report(const char *, ...) {}

static void resetImage() {
    std::fill(image.begin(), image.end(), 0);
    auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(image.data());
    dos->e_lfanew = 0x80;
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(image.data() + dos->e_lfanew);
    nt->FileHeader.NumberOfSections = 2;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
    auto *section = IMAGE_FIRST_SECTION(nt);
    section[0].VirtualAddress = 0x300;
    section[0].Misc.VirtualSize = 256;
    section[0].Characteristics = IMAGE_SCN_MEM_EXECUTE;
    section[1].VirtualAddress = 0x500;
    section[1].Misc.VirtualSize = 256;
}

static void bytes(size_t offset, std::initializer_list<unsigned char> value) {
    std::copy(value.begin(), value.end(), image.begin() + offset);
}

int main() {
    resetImage();
    assert(HostBindings::scan("A1 ? ? ? ? C3", 1, report) == 0);
    bytes(0x300, {0xa1, 0x44, 0x33, 0x22, 0x11, 0xc3});
    assert(HostBindings::scan("A1 ? ? ? ? C3", 1, report) == 0x11223344);
    bytes(0x500, {0xa1, 0x99, 0x88, 0x77, 0x66, 0xc3});
    assert(HostBindings::scan("A1 ? ? ? ? C3", 1, report) == 0x11223344);
    bytes(0x320, {0xa1, 0x44, 0x33, 0x22, 0x11, 0xc3});
    assert(HostBindings::scan("A1 ? ? ? ? C3", 1, report) == 0);

    resetImage();
    HostBindings::Slots slots;
    assert(!slots.discover(report));
    bytes(0x300, {0x8b, 0x15, 0x44, 0x33, 0x22, 0x11, 0xf3, 0x0f, 0x10, 0x44, 0x24, 0x30, 0x52});
    bytes(0x320, {0xd9, 0x45, 0x08, 0xa1, 0x88, 0x77, 0x66, 0x55, 0x51, 0xd9, 0x1c, 0x24, 0x50});
    assert(!slots.discover(report)); // Required map slot is still absent.
    bytes(0x340, {0x8b, 0x48, 0x04, 0x8b, 0x11, 0x50, 0x8b, 0x42, 0x34, 0xff, 0xd0, 0xc7, 0x46,
                  0x2c, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x8b, 0x0d, 0xcc, 0xbb, 0xaa, 0x99, 0x8b,
                  0x89, 0x50, 0x0b, 0x00, 0x00, 0x32, 0xc0, 0x83, 0x79, 0x28, 0x00});
    assert(slots.discover(report)); // Event flags are optional at startup.
    assert(slots.player == 0x11223344 && slots.camera == 0x55667788);
    assert(slots.map == 0x99aabbcc && slots.eventFlags == 0);
    bytes(0x380,
          {0x56, 0x8b, 0xf1, 0x8b, 0x46, 0x1c, 0x50, 0xa1, 0x04, 0x03, 0x02, 0x01, 0x32, 0xc9});
    assert(slots.discover(report) && slots.eventFlags == 0x01020304);
    puts("PASSED: native slot discovery in a synthetic executable image");
}
