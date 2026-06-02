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

// Reads keyboard, drives FLAG_PLAYER entities: horizontal move, jump (only
// when grounded), facing flip, and animation state selection.
void inputUpdate(World& w, const PlayerConfig& cfg, const PlayerAnims& anims);

} // namespace sc
