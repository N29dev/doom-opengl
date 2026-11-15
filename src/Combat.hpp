#pragma once
#include <algorithm>
namespace combat {
inline constexpr int titanHealth = 850;
inline constexpr int rayCount(int weapon) { return weapon == 2 ? 5 : 1; }
inline int hitDamage(int weapon, bool titan, float distance, float range) {
    if (weapon != 2) return titan ? 25 : 40;
    const float falloff = std::clamp(1.0f - distance / range, 0.0f, 1.0f);
    return static_cast<int>(12.0f + 43.0f * falloff);
}
}
