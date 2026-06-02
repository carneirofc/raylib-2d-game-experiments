#pragma once
#include "world/World.hpp"
#include "gfx/AnimationBank.hpp"
#include "gfx/TextureCache.hpp"
#include <raylib.h>

namespace sc {

// Draws every active, on-screen entity from its current animation frame.
// Off-screen entities (outside the camera view + a margin) are culled.
// Returns the number of entities actually drawn (for the debug HUD).
int renderEntities(const World& w,
                   const AnimationBank& bank,
                   const TextureCache& textures,
                   const Camera2D& cam);

} // namespace sc
