#pragma once
#include <cstdint>

// Owned PTDE passive ESD layer 9; category-2 reactions are observed on track 25.
// Excludes guard breaks/parries (2080+),
// paired throws, death and unknown animation families from acceleration.
namespace PTDEReaction {
enum class Kind { Other, Flinch, Knockdown };

inline int passiveAnimation(int legacy, uint32_t legacyToken, int categorized,
                            uint32_t categorizedToken) {
    if (categorizedToken != 0xffffffffu && categorized >= 0 && categorized % 10000 >= 2000 &&
        categorized % 10000 <= 2100)
        return categorized % 10000;
    return legacyToken != 0xffffffffu && legacy >= 0 ? legacy % 10000 : -1;
}

inline Kind classify(int animation) {
    if ((animation >= 2000 && animation <= 2007) || (animation >= 2020 && animation <= 2022) ||
        (animation >= 2030 && animation <= 2037))
        return Kind::Flinch;
    if ((animation >= 2010 && animation <= 2017) || (animation >= 2024 && animation <= 2027) ||
        animation == 2040 || animation == 2045 || (animation >= 2050 && animation <= 2057))
        return Kind::Knockdown;
    return Kind::Other;
}

inline float rate(Kind kind, bool running, bool paired, float flinch, float knockdown) {
    if (!running || paired)
        return 1;
    return kind == Kind::Flinch ? flinch : kind == Kind::Knockdown ? knockdown : 1;
}
} // namespace PTDEReaction
