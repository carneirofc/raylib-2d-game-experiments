#pragma once
#include "world/World.hpp"
#include "gfx/AnimationBank.hpp"

namespace sc {

// Animation ids the player uses, resolved once at startup.
struct PlayerAnims {
    int idle = -1;
    int run  = -1;
    int jump = -1;
};

struct PlayerConfig {
    float moveSpeed = 220.0f; // px/s horizontal
    float jumpSpeed = 520.0f; // px/s initial upward
};

// Hardware -> Intent. Reads keyboard, writes intent[] for FLAG_PLAYER entities.
// No gameplay knowledge — enemies get the same Intent from aiUpdate instead, and
// the downstream control/weapon code can't tell the two sources apart.
void inputUpdate(World& w);

// Intent -> effects. Consumes intent[] for every actor (FLAG_PLAYER|FLAG_ENEMY):
// horizontal velocity, jump (only when grounded), facing flip, and animation
// selection. Firing is handled separately by weaponUpdate (it needs to spawn).
void controlUpdate(World& w, const PlayerConfig& cfg, const PlayerAnims& anims);

} // namespace sc
