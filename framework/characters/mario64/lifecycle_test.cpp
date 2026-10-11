#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "../../sdk/character_api.h"

static std::string lastMessage;

static void captureLog(const char *message) {
    lastMessage = message;
}

int main(int argc, char **argv) {
    if (argc != 4)
        return 2;
    HMODULE module = LoadLibraryA(argv[1]);
    assert(module);
    auto entry = reinterpret_cast<SC_GetAPI>(GetProcAddress(module, "SoulsCharacterGetAPI"));
    assert(entry);
    const auto *api = entry(SC_ABI_VERSION, sizeof(SC_CharacterAPI));
    assert(api);
    const auto *profile = api->describe();
    std::vector<uint8_t> pixels(profile->textureWidth * profile->textureHeight * 4);
    SC_Texture texture = {pixels.data(), uint32_t(pixels.size())};
    const std::string missingRuntime = std::string(argv[3]) + ".not-present";
    const std::string missingRom = std::string(argv[2]) + ".not-present";
    assert(GetFileAttributesA(missingRuntime.c_str()) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesA(missingRom.c_str()) == INVALID_FILE_ATTRIBUTES);

    // A failure at each resource-acquisition stage must leave a retry possible.
    // The adapter itself is a valid DLL without guest-runtime exports, so this
    // also exercises symbol refusal without a separately maintained fake DLL.
    for (int cycle = 0; cycle < 2; cycle++) {
        SC_Init init = {sizeof(SC_Init), argv[2], missingRuntime.c_str(), captureLog};
        assert(!api->initialize(&init, &texture));
        assert(lastMessage == "Mario runtime could not be loaded");
        assert(api->create(0, 0, 0, 0) < 0);
        api->shutdown();

        init.runtimePath = argv[1];
        assert(!api->initialize(&init, &texture));
        assert(lastMessage == "Missing sm64_global_init");
        api->shutdown();

        init.runtimePath = argv[3];
        init.assetPath = missingRom.c_str();
        assert(!api->initialize(&init, &texture));
        assert(lastMessage == "Owned US Mario 64 ROM is missing");
        api->shutdown();

        init.assetPath = argv[2];
        assert(api->initialize(&init, &texture));
        assert(!api->initialize(&init, &texture));
        int actor = api->create(0, 0, 0, 0);
        assert(actor >= 0);
        assert(api->create(0, 0, 0, 0) < 0);
        // Shutdown must release a still-live instance as well as loaded assets.
        api->shutdown();
        api->shutdown();
        assert(api->create(0, 0, 0, 0) < 0);
    }
    FreeLibrary(module);
    puts("PASS Mario runtime acquisition failures, retry, single-instance ownership and shutdown");
}
