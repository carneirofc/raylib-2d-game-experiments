#pragma once
#include "scene/Scene.hpp"

namespace sc {

// Turns each actor's Intent.fire into bullets, gated by a per-entity cooldown.
// Lives at scene level (not systems/) because firing allocates entities and
// emits sound — it needs the whole Scene, not just the World arrays.
void weaponUpdate(Scene& s, float dt);

// Entity-vs-entity hit resolution: every live bullet is tested against the
// opposing-team actors (O(bullets x actors); fine at skeleton scale). On a hit
// it applies damage, queues the bullet for destruction, and either respawns the
// player or destroys a dead enemy. Bullets-vs-static-solids is handled in
// collisionUpdate instead.
void combatUpdate(Scene& s);

} // namespace sc
