#pragma once
#include "world/World.hpp"
#include "systems/Movement.hpp"
#include "systems/Input.hpp"
#include "systems/Juice.hpp"

namespace sc {

// Live debug state the HUD reads/writes. Owned by Game.
struct DebugState {
    bool  showHud         = true;
    bool  showSpriteEditor = false;
    int   spawnRequest    = 0;     // crowd entities the HUD wants spawned this frame
    int   despawnRequest  = 0;
    int   enemyRequest    = 0;     // enemies to spawn near the player this frame
    int   enemyBehavior   = AI_CHASER; // which brain the spawned enemies get
    int   drawnLastFrame  = 0;
    Entity inspected      = INVALID_ENTITY;
};

// Renders the main debug HUD: FPS/frame-time, entity counts, spawn/despawn
// controls, physics tuning, input state + buffer, squash/stretch (juice) tuning,
// and a simple entity inspector. ImGui frame must already be begun by the caller.
void debugHud(DebugState& dbg, World& w, Physics& phys, PlayerConfig& player,
              InputState& input, JuiceConfig& juice);

} // namespace sc
