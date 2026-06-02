#pragma once
#include "world/World.hpp"
#include "gfx/AnimationBank.hpp"

namespace sc {

// Advances every active entity's animation frame by dt. Pure linear pass over
// the SoA arrays — no allocation, no string lookups.
void animatorUpdate(World& w, const AnimationBank& bank, float dt);

// Switch an entity to a new animation, resetting playback. No-op if already
// playing that animation (so it doesn't stutter when held).
void playAnim(World& w, EntityId e, int animId);

// Step a single AnimState forward (shared by runtime + SpriteEditor preview).
void stepAnim(AnimState& st, const AnimationDef& def, float dt);

} // namespace sc
