#pragma once

// Observe the game's factory instead of manufacturing a probe device. Graphics
// wrappers may bind global resources and count factory calls: a probe is not inert.
namespace HostGraphics {
using Factory = IDirect3D9 *(WINAPI *)(UINT);
using CreateDevice = HRESULT(WINAPI *)(IDirect3D9 *, UINT, D3DDEVTYPE, HWND, DWORD,
                                       D3DPRESENT_PARAMETERS *, IDirect3DDevice9 **);
static Factory originalFactory = nullptr;
static CreateDevice originals[2]{};
static void *entries[2]{};
using Observe = void (*)(IDirect3DDevice9 *);
static Observe observer = nullptr;

// DSfix forwards QueryInterface to its underlying device. Use the public COM
// contract rather than depending on private wrapper layouts. The returned
// reference is temporary; hook code owns no reference to the game device.
static void observeDevice(IDirect3DDevice9 *device) {
    if (!device || !observer)
        return;
    IDirect3DDevice9 *native = nullptr;
    if (SUCCEEDED(
            device->QueryInterface(IID_IDirect3DDevice9, reinterpret_cast<void **>(&native))) &&
        native) {
        observer(native);
        native->Release();
    }
}

template <unsigned Slot>
static HRESULT WINAPI createDevice(IDirect3D9 *self, UINT adapter, D3DDEVTYPE type, HWND window,
                                   DWORD flags, D3DPRESENT_PARAMETERS *parameters,
                                   IDirect3DDevice9 **out) {
    HRESULT hr = originals[Slot](self, adapter, type, window, flags, parameters, out);
    if (SUCCEEDED(hr) && out)
        observeDevice(*out);
    return hr;
}

static bool watchFactory(IDirect3D9 *factory) {
    if (!factory)
        return false;
    void *entry = (*reinterpret_cast<void ***>(factory))[16];
    for (unsigned i = 0; i < 2; ++i) {
        if (entries[i] == entry)
            return true;
        if (entries[i])
            continue;
        void *hook = i == 0 ? reinterpret_cast<void *>(createDevice<0>)
                            : reinterpret_cast<void *>(createDevice<1>);
        auto init = MH_Initialize();
        if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED)
            return false;
        if (MH_CreateHook(entry, hook, reinterpret_cast<void **>(&originals[i])) != MH_OK)
            return false;
        if (MH_EnableHook(entry) != MH_OK) {
            MH_RemoveHook(entry);
            originals[i] = nullptr;
            return false;
        }
        entries[i] = entry;
        return true;
    }
    return false; // Unknown extra factory implementation: never replace a trampoline.
}

static IDirect3D9 *WINAPI factory(UINT version) {
    IDirect3D9 *result = originalFactory ? originalFactory(version) : nullptr;
    if (result && !watchFactory(result))
        logMessage("Graphics factory hook unavailable; native rendering retained");
    return result;
}

// Patch only the executable's named import. Called during loader initialization,
// before the main thread can create its graphics device. No graphics calls,
// library loading, waiting, or MinHook initialization happen under the loader lock.
static bool install(HMODULE executable, Observe callback) {
    if (originalFactory)
        return false; // Never chain our own interceptor.
    auto base = reinterpret_cast<BYTE *>(executable);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS32 *>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return false;
    auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress || directory.Size < sizeof(IMAGE_IMPORT_DESCRIPTOR))
        return false;
    auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + directory.VirtualAddress);
    for (unsigned i = 0; i < directory.Size / sizeof(*imports) && imports[i].Name; ++i) {
        if (_stricmp(reinterpret_cast<const char *>(base + imports[i].Name), "d3d9.dll"))
            continue;
        if (!imports[i].OriginalFirstThunk || !imports[i].FirstThunk)
            return false;
        auto names = reinterpret_cast<IMAGE_THUNK_DATA32 *>(base + imports[i].OriginalFirstThunk);
        auto addresses = reinterpret_cast<IMAGE_THUNK_DATA32 *>(base + imports[i].FirstThunk);
        for (unsigned j = 0; names[j].u1.AddressOfData; ++j) {
            if (IMAGE_SNAP_BY_ORDINAL32(names[j].u1.Ordinal))
                continue;
            auto name = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base + names[j].u1.AddressOfData);
            if (strcmp(reinterpret_cast<const char *>(name->Name), "Direct3DCreate9"))
                continue;
            auto slot = reinterpret_cast<void **>(&addresses[j].u1.Function);
            DWORD protection = 0;
            if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protection))
                return false;
            observer = callback;
            originalFactory = reinterpret_cast<Factory>(*slot);
            InterlockedExchangePointer(slot, reinterpret_cast<void *>(factory));
            DWORD ignored;
            VirtualProtect(slot, sizeof(*slot), protection, &ignored);
            return true;
        }
    }
    return false;
}
} // namespace HostGraphics
