#pragma once
// PTDE's native effect constructor. Action effects are observed independently
// of item consumption/healing, which remain entirely inside the native engine.
using ActionEffectFn = uintptr_t(__cdecl *)(uintptr_t, uintptr_t, int, uintptr_t);
static ActionEffectFn actionOriginalEffect = nullptr;

static uintptr_t __cdecl actionEffect(uintptr_t out, uintptr_t actor, int id, uintptr_t setup) {
    // c0000 a00:7585/7586/7587 use 227 at dummy 26 for the flask mesh.
    // PTDE resolves the test character's variant to 22750. 229 is the drink
    // spill at the same human-hand dummy, so replace that visual too. 228 is
    // the body healing effect and remains native. No item or HP logic is here.
    // Passing -1 uses the native empty-handle constructor path.
    if (characterOwned() && hideNative && (profile->capabilities & SC_CAP_ITEM_PROP) &&
        actor == player && (id == 22700 || id == 22750 || id == 22900 || id == 22950))
        id = -1;
    return actionOriginalEffect(out, actor, id, setup);
}

static void installActionEffects() {
    const unsigned char expected[] = {0x53, 0x8b, 0xdc, 0x83, 0xec, 8, 0x83, 0xe4, 0xf0};
    unsigned char actual[sizeof(expected)] = {};
    if (!get(uintptr_t(0xd34e40), actual) || memcmp(actual, expected, sizeof(expected)))
        return;
    if (MH_CreateHook((void *)0xd34e40, (void *)actionEffect, (void **)&actionOriginalEffect) ==
        MH_OK)
        logf("Action effect hook=%d", MH_EnableHook((void *)0xd34e40));
}
