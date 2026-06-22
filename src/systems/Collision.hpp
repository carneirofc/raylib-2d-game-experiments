#pragma once
#include "world/World.hpp"
#include "scene/Level.hpp"
#include "scene/SpatialGrid.hpp"
#include "systems/Juice.hpp" // JuiceConfig, juiceKick (landing squash)
#include <raylib.h>

namespace sc {

// Resolves every active entity's AABB against the level solids, using the grid
// broad-phase to test only nearby solids. Uses the smaller overlap axis to push
// out (so entities slide along surfaces and land cleanly), sets/clears
// FLAG_GROUNDED, and zeroes velocity on the contact axis. The downward impact
// speed at first floor contact kicks the squash/stretch spring (juice).
void collisionUpdate(World& w, const Level& lv, const SpatialGrid& grid,
                     const JuiceConfig& juice);

} // namespace sc
