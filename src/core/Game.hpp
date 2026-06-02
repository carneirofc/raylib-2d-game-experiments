#pragma once
#include "core/Time.hpp"
#include "world/World.hpp"
#include "gfx/TextureCache.hpp"
#include "gfx/AnimationBank.hpp"
#include "systems/Movement.hpp"
#include "systems/Input.hpp"
#include "systems/Collision.hpp"
#include "debug/DebugUI.hpp"
#include "debug/SpriteEditor.hpp"
#include <raylib.h>

namespace sc {

// All game state in one struct. Free functions below drive it.
struct Game {
    World         world;
    TextureCache  textures;
    AnimationBank bank;
    TileMap       map;
    Camera2D      cam{};

    Physics       phys{};
    PlayerConfig  player{};
    PlayerAnims   playerAnims{};
    int           crowdAnimId = -1; // default anim for spawned crowd

    DebugState    dbg{};
    SpriteEditor  editor{};

    EntityId      playerEntity = INVALID_ENTITY;
    FixedTimestep clock{};
};

void gameInit(Game& g, int width, int height, const char* title);
void gameRun(Game& g);
void gameShutdown(Game& g);

} // namespace sc
