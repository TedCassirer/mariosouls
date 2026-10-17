#pragma once
#include "../../core/unarmed_equipment.h"
// Normal Caestus retains native reinforcement, embers, inventory and saves.
// Infusions deliberately do not map to guest attack progression.
static constexpr int unarmedNativeItem = 901000;
static SC_UnarmedEquipmentProfile unarmedProfile{};
static bool unarmedEquipmentEnabled = false;
static wchar_t unarmedName[48]{}, unarmedNames[16][64]{}, unarmedSummary[128]{},
    unarmedDescription[384]{};

static void unarmedEquipmentConfigure() {
    auto entry = (SC_GetUnarmedEquipmentProfile)GetProcAddress(
        adapter.module, "SoulsCharacterGetUnarmedEquipmentProfile");
    auto p = entry ? entry(SC_UNARMED_EQUIPMENT_PROFILE_VERSION, sizeof(unarmedProfile)) : nullptr;
    unarmedEquipmentEnabled = SCUnarmed::valid(p);
    if (!unarmedEquipmentEnabled)
        return;
    unarmedProfile = *p;
    unarmedEquipmentEnabled =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p->name, -1, unarmedName, 48) > 0 &&
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p->summary, -1, unarmedSummary, 128) >
            0 &&
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p->description, -1, unarmedDescription,
                            384) > 0;
    if (unarmedEquipmentEnabled) {
        wcscpy(unarmedNames[0], unarmedName);
        for (int level = 1; level <= 15; ++level)
            swprintf(unarmedNames[level], 64, L"%ls+%d", unarmedName, level);
    }
}

static int unarmedEquipmentLevel(const NativeEquipment &gear) {
    return gear.valid && gear.right.valid && !gear.right.empty &&
                   gear.right.rowId == unarmedNativeItem && gear.right.category == 9 &&
                   gear.right.animation == 42 && gear.right.upgrade >= 0 && gear.right.upgrade <= 15
               ? gear.right.upgrade
               : -1;
}

static uintptr_t unarmedEquipmentText(int id, uintptr_t fmg, bool applicable) {
    if (!unarmedEquipmentEnabled || !applicable || id < unarmedNativeItem ||
        id > unarmedNativeItem + 15 || !fmg)
        return 0;
    uintptr_t repo = read<uint32_t>(0x13785dc);
    if (!repo)
        return 0;
    // Owned executable D1A800 / D1A7B0 / D1A760: base + patch FMG slots.
    if (fmg == read<uint32_t>(repo + 0x30) || fmg == read<uint32_t>(repo + 0x1d0))
        return (uintptr_t)unarmedNames[id - unarmedNativeItem];
    if (fmg == read<uint32_t>(repo + 0x58) || fmg == read<uint32_t>(repo + 0x1cc))
        return (uintptr_t)unarmedSummary;
    if (fmg == read<uint32_t>(repo + 0x68) || fmg == read<uint32_t>(repo + 0x1ac))
        return (uintptr_t)unarmedDescription;
    return 0;
}

static uintptr_t unarmedEquipmentParam(int &cachedSlot, const char *schema, int id) {
    if (uintptr_t row = equipmentParamRow(cachedSlot, schema, id))
        return row;
    // Binder IDs do not match repository slots. Discover using validated schema.
    for (int slot = 0; slot < 36; ++slot)
        if (uintptr_t row = equipmentParamRow(slot, schema, id)) {
            cachedSlot = slot;
            return row;
        }
    return 0;
}

static void unarmedStartingEquipment(bool enabled) {
    if (!enabled || !unarmedEquipmentEnabled)
        return;
    // New class and class-specific pickup only; never edit live inventory/stats.
    // Re-resolve storage on reload. Refuse other mods' replacement items.
    static int initSlot = -1, lotSlot = -1;
    uintptr_t init = unarmedEquipmentParam(initSlot, "CHARACTER_INIT_PARAM", 3007);
    uintptr_t lot = unarmedEquipmentParam(lotSlot, "ITEMLOT_PARAM_ST", 1810260);
    int weapon = -1, pickup = -1;
    if (!init || !lot || !get(init + 0x10, weapon) || !get(lot, pickup) ||
        (weapon != 700000 && weapon != unarmedNativeItem) ||
        (pickup != 700000 && pickup != unarmedNativeItem))
        return;
    if (weapon == 700000 && put(init + 0x10, unarmedNativeItem))
        logMessage("UNARMED new-class right weapon mapped to native fist item");
    if (pickup == 700000 && put(lot, unarmedNativeItem))
        logMessage("UNARMED class tutorial weapon mapped to native fist item");
}
