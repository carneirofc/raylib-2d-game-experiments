#pragma once
#include "scene/Scene.hpp"
#include <raylib.h>

namespace sc {

// Entity factory over a scene's world. Spawned entities use the scene's default
// crowd animation; the player gets FLAG_PLAYER and its idle animation.
Entity spawnPlayer(Scene& s, Vector2 at);

// Create one AI-driven enemy at `at` (FLAG_ENEMY, TEAM_ENEMY, full health) with
// the given brain (see AIBehavior). Flyers also get FLAG_NOGRAVITY.
Entity spawnEnemy(Scene& s, Vector2 at, std::uint8_t behavior = AI_CHASER);

// Fire a bullet from `shooter` travelling in `dir` (-1 left / +1 right). The
// bullet inherits the shooter's team so it can't hit its own side.
Entity spawnBullet(Scene& s, EntityIndex shooter, float dir);

// Deterministic scatter of n crowd entities (index hash, no RNG).
void   spawnCrowd(Scene& s, int n);

// Destroy up to n non-player entities (handle compare skips the player).
void   despawnCrowd(Scene& s, int n);

} // namespace sc
