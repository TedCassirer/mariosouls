#pragma once
#include "../sdk/character_api.h"

namespace SCState {
// A native-owned pose may update geometry without exposing offensive guest state.
// Preserve diagnostic/action/position fields; only combat eligibility is cleared.
inline void clearCombat(SC_State &state) {
    state.flags = 0;
    state.attackId = state.interruptedAttackId = -1;
    state.landingRecoveryTicks = 0;
}
} // namespace SCState
