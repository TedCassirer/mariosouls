#pragma once

namespace SCQA {
// A fixed, user-authorized melee fixture. This is not a widened level range.
inline bool meleeFixture(int level, int vitality, int attunement, int endurance, int strength,
                         int dexterity, int resistance, int intelligence, int faith) {
    return level == 13 && vitality == 11 && attunement == 11 && endurance == 11 && strength == 16 &&
           dexterity == 13 && resistance == 11 && intelligence == 11 && faith == 11;
}
} // namespace SCQA
