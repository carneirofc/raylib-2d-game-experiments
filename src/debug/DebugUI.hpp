#pragma once
#include "world/World.hpp"
#include "systems/Movement.hpp"
#include "systems/Input.hpp"

namespace sc {

// Live debug state the HUD reads/writes. Owned by Game.
struct DebugState {
    bool  showHud         = true;
    bool  showSpriteEditor = false;
    int   spawnRequest    = 0;     // entities the HUD wants spawned this frame
    int   despawnRequest  = 0;
    int   drawnLastFrame  = 0;
    EntityId inspected    = INVALID_ENTITY;
};

// Renders the main debug HUD: FPS/frame-time, entity counts, spawn/despawn
// controls, physics tuning, and a simple entity inspector. ImGui frame must
// already be begun by the caller.
void debugHud(DebugState& dbg, World& w, Physics& phys, PlayerConfig& player);

} // namespace sc
