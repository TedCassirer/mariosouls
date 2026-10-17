#pragma once

static const char *attackName(int id) {
    return id == SC_MAX_ATTACKS ? "equipped_sword"
           : id >= 0 && adapter.profile && id < int(adapter.profile->attackCount)
               ? adapter.profile->attacks[id].name
               : "none";
}

static const SC_Attack *attackDefinition(int id) {
    return id >= 0 && adapter.profile && id < int(adapter.profile->attackCount)
               ? &adapter.profile->attacks[id]
               : nullptr;
}

static bool automaticContact(Vec delta, const SC_Attack &a) {
    return delta.y > a.automaticMinHeightCm * .01f && delta.y < a.automaticMaxHeightCm * .01f &&
           delta.x * delta.x + delta.z * delta.z <
               a.automaticRadiusCm * a.automaticRadiusCm * .0001f;
}

static void adapterLog(const char *message) {
    logMessage("ADAPTER %s", message ? message : "");
}

static bool initCharacter() {
    char id[48] = {}, file[MAX_PATH] = {}, defaultPath[MAX_PATH] = {}, section[80] = {},
         asset[MAX_PATH] = {}, runtime[MAX_PATH] = {};
    // Retain the configuration filename and gameplay keys for existing players.
    GetPrivateProfileStringA("Framework", "Character", "mario64", id, sizeof(id), ini);
    if (!SC::identifier(id, sizeof(id))) {
        logMessage("Invalid character identifier");
        return false;
    }
    snprintf(defaultPath, sizeof(defaultPath),
             "%s\\SoulsCharacterFramework\\characters\\%s\\character.dll", root, id);
    GetPrivateProfileStringA("Framework", "Adapter", defaultPath, file, sizeof(file), ini);
    adapter.module = LoadLibraryExA(file, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!adapter.module) {
        logMessage("Character adapter missing: %s error=%lu", file, GetLastError());
        return false;
    }
    auto entry = (SC_GetAPI)GetProcAddress(adapter.module, "SoulsCharacterGetAPI");
    adapter.api = entry ? entry(SC_ABI_VERSION, sizeof(SC_CharacterAPI)) : nullptr;
    if (!SC::api(adapter.api) || !SC::descriptor(adapter.api->describe())) {
        logMessage("Character ABI/profile rejected");
        FreeLibrary(adapter.module);
        adapter.module = nullptr;
        adapter.api = nullptr;
        return false;
    }
    adapter.profile = adapter.api->describe();
    if (strcmp(adapter.profile->id, id)) {
        logMessage("Character identity mismatch");
        FreeLibrary(adapter.module);
        adapter.module = nullptr;
        adapter.api = nullptr;
        adapter.profile = nullptr;
        return false;
    }
    adapter.mesh.allocate(*adapter.profile);
    snprintf(section, sizeof(section), "Character.%s", id);
    GetPrivateProfileStringA("MarioSouls", "ROM", "", defaultPath, sizeof(defaultPath), ini);
    GetPrivateProfileStringA(section, "Asset", defaultPath, asset, sizeof(asset), ini);
    GetPrivateProfileStringA("MarioSouls", "Library", "", defaultPath, sizeof(defaultPath), ini);
    GetPrivateProfileStringA(section, "Runtime", defaultPath, runtime, sizeof(runtime), ini);
    SC_Init init = {sizeof(SC_Init), asset, runtime, adapterLog};
    SC_Texture atlas = {adapter.mesh.atlasPixels.data(), uint32_t(adapter.mesh.atlasPixels.size())};
    if (!adapter.api->initialize(&init, &atlas)) {
        logMessage("Character initialization failed");
        adapter.api->shutdown();
        FreeLibrary(adapter.module);
        adapter.module = nullptr;
        adapter.api = nullptr;
        adapter.profile = nullptr;
        return false;
    }
    const unsigned char prefix[] = {0xd9, 0x44, 0x24, 0x18, 0x8b, 0x44,
                                    0x24, 0x0c, 0x8b, 0x4c, 0x24, 0x04};
    unsigned char actual[sizeof(prefix)] = {};
    if (get(uintptr_t(0xd65dc0), actual) && memcmp(actual, prefix, sizeof(prefix)) == 0)
        nativeDamage = (DamageFn)0xd65dc0;
    characterBody = adapter.profile->body;
    combatSettings();
    cameraConfigure();
    projectileConfigure();
    impactConfigure();
    defenseConfigure();
    environmentConfigure();
    launcherConfigure();
    classConfigure();
    unarmedEquipmentConfigure();
    if (adapter.profile->capabilities & SC_CAP_AUDIO) {
        WAVEFORMATEX format = {};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2;
        format.nSamplesPerSec = adapter.profile->audioRate;
        format.wBitsPerSample = 16;
        format.nBlockAlign = 4;
        format.nAvgBytesPerSec = format.nSamplesPerSec * 4;
        if (waveOutOpen(&audioOut, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
            waveOutSetVolume(audioOut, 0x60006000);
            for (int i = 0; i < 4; i++) {
                audioHeaders[i].lpData = (LPSTR)audioBuffers[i];
                audioHeaders[i].dwBufferLength = sizeof(audioBuffers[i]);
                waveOutPrepareHeader(audioOut, &audioHeaders[i], sizeof(WAVEHDR));
            }
        } else
            audioOut = nullptr;
    }
    logMessage("Framework ABI=%u character=%s attacks=%u atlas=%ux%u ready", SC_ABI_VERSION,
               adapter.profile->id, adapter.profile->attackCount, adapter.profile->textureWidth,
               adapter.profile->textureHeight);
    return true;
}
