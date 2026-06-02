#pragma once
#include "world/World.hpp"
#include <string>

namespace sc {

// Shared tunables for the enemy brains. Per-entity behaviour selection lives in
// World.ai[e].behavior; these ranges/speeds are global (move any of them into
// AIState if you want per-enemy tuning). All distances in world pixels.
struct AIConfig {
    float aggroRange = 360.0f; // engage the player within this distance
    float fireRange  = 300.0f; // shoot once at least this close
    float jumpReach  = 48.0f;  // hop if the player is this much higher (grounded brains)
};

// Signature every behaviour implements: read the world + the (already validated)
// player slot, write this enemy's Intent for the tick. Pure over the arrays.
using AIBehaviorFn = void (*)(World& w, EntityIndex self, EntityIndex player, const AIConfig& cfg);

// Enemy brains: clears each enemy's Intent, then dispatches to the function its
// AIState.behavior selects. Runs BEFORE controlUpdate, which turns the Intent
// into motion identically for players and enemies. `player` is the target's raw
// slot (INVALID_INDEX -> behaviours fall back to passive/patrol).
void aiUpdate(World& w, EntityIndex player, const AIConfig& cfg);

// Map a level-file "type" string ("patrol","chaser","sentry","flyer","coward")
// to an AIBehavior. Unknown / empty -> AI_CHASER.
std::uint8_t aiBehaviorFromName(const std::string& name);

} // namespace sc
