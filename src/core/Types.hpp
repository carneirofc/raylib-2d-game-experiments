#pragma once
#include <cstdint>
#include <limits>

namespace sc {

// Plain 2D float pair. No methods, no operators — just data.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

using EntityId = std::uint32_t;
inline constexpr EntityId INVALID_ENTITY = std::numeric_limits<EntityId>::max();

} // namespace sc
