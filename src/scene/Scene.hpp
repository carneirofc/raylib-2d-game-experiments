#pragma once
#include "world/World.hpp"
#include "scene/Level.hpp"
#include "scene/SpatialGrid.hpp"
#include "scene/LevelLoad.hpp"
#include "gfx/TextureCache.hpp"
#include "gfx/AnimationBank.hpp"
#include "audio/SoundCache.hpp"
#include "systems/Movement.hpp"   // Physics
#include "systems/Input.hpp"      // PlayerConfig, PlayerAnims
#include "systems/AI.hpp"         // AIConfig
#include <raylib.h>
#include <vector>

namespace sc {

// Weapon tunables shared by every shooter (player and enemies).
struct Weapon {
    float fireCooldown = 0.18f;  // seconds between shots
    float bulletSpeed  = 700.0f; // px/s
    float bulletTTL    = 1.5f;   // seconds before a bullet expires
    float muzzleOffset = 22.0f;  // spawn this far ahead of the shooter centre
};

// Damage + hit-point tunables, read by combatUpdate.
struct CombatConfig {
    int bulletDamage    = 10;
    int playerMaxHealth = 100;
    int enemyHealth     = 30;
};

// Owns all gameplay state and runs the fixed-step system schedule. Knows nothing
// about windowing, ImGui, or the debug UI — those wrap the scene from outside.
struct Scene {
    World         world;
    Level         level;
    SpatialGrid   grid;
    Camera2D      cam{};
    TextureCache  textures;
    AnimationBank bank;
    SoundCache    sounds;

    Physics       phys{};
    PlayerConfig  player{};
    PlayerAnims   playerAnims{};
    Weapon        weapon{};
    CombatConfig  combat{};
    AIConfig      ai{};
    int           crowdAnimId = -1;
    LevelSpawn    spawn{};               // parsed bootstrap parameters

    Entity        playerEntity = INVALID_ENTITY;

    // Sound effects requested this tick (opaque SoundCache ids), drained by
    // sfxFlush so systems never touch the audio device directly.
    std::vector<std::uint16_t> sfxQueue;
    std::uint16_t sfxShoot = 0, sfxHit = 0, sfxEnemyDie = 0;
};

// Queue a sound to play at the end of this tick.
inline void sceneSfx(Scene& s, std::uint16_t id) { s.sfxQueue.push_back(id); }

// Load assets + level, build the grid, create the player, crowd, and enemies.
void sceneInit(Scene& s, int screenW, int screenH,
               const char* manifestPath, const char* levelPath);

// One simulation tick: the documented system schedule (see Scene.cpp).
void sceneFixedUpdate(Scene& s, float dt);

// Draw the world + level into the camera. NO ImGui. outDrawn = entities drawn.
void sceneRender(Scene& s, int& outDrawn);

// Release GPU/audio resources owned by the scene.
void sceneShutdown(Scene& s);

} // namespace sc
