#pragma once
#include "world/World.hpp"

namespace sc {

struct Physics {
    float gravity = 1400.0f; // px/s^2, downward (+y)
    float maxFall = 1600.0f; // terminal velocity
};

// Integrates velocity (with gravity) into position over a fixed step.
// Pure linear pass over the SoA arrays.
void movementUpdate(World& w, const Physics& phys, float dt);

} // namespace sc
