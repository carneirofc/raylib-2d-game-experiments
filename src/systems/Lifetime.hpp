#pragma once
#include "world/World.hpp"

namespace sc {

// Ticks down lifetime[] for entities that have one (>= 0) and queues any that
// reach zero for destruction. Entities with lifetime < 0 are persistent and
// skipped. Run late in the schedule, before worldFlushDestroys.
void lifetimeUpdate(World& w, float dt);

} // namespace sc
