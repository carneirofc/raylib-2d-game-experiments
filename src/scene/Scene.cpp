#include "scene/Scene.hpp"
#include "scene/Spawn.hpp"
#include "scene/Combat.hpp"
#include "scene/AssetManifest.hpp"
#include "systems/Collision.hpp"
#include "systems/Animator.hpp"
#include "systems/Lifetime.hpp"
#include "systems/Render.hpp"
#include "systems/Camera.hpp"

namespace sc {

// Play and clear every sound queued by this tick's systems. The single point
// that touches the audio device, keeping systems hardware-free like Input.
static void sfxFlush(Scene& s) {
    for (std::uint16_t id : s.sfxQueue) sfxPlay(s.sounds, id);
    s.sfxQueue.clear();
}

void sceneInit(Scene& s, int screenW, int screenH,
               const char* manifestPath, const char* levelPath) {
    worldInit(s.world, 8192);
    manifestLoad(s.bank, s.textures, manifestPath, s.playerAnims, s.crowdAnimId);
    levelLoad(s.level, s.spawn, levelPath);
    gridBuild(s.grid, s.level, s.spawn.cellSize);

    // Sound effects (optional: a missing .wav just plays silent — see SoundCache).
    s.sfxShoot    = sfxLoad(s.sounds, "assets/sounds/shoot.wav");
    s.sfxHit      = sfxLoad(s.sounds, "assets/sounds/hit.wav");
    s.sfxEnemyDie = sfxLoad(s.sounds, "assets/sounds/enemy_die.wav");

    s.playerEntity = spawnPlayer(s, s.spawn.player);
    spawnCrowd(s, s.spawn.crowd);
    for (const EnemySpawn& en : s.spawn.enemies) spawnEnemy(s, en.pos, en.behavior);

    s.cam.zoom   = 1.0f;
    s.cam.offset = {screenW * 0.5f, screenH * 0.5f};
}

void sceneFixedUpdate(Scene& s, float dt) {
    // Schedule order is load-bearing:
    //   ai       -> writes enemy Intent toward the player (patrol when far)
    //   input    -> writes player Intent from hardware
    //   control  -> turns every actor's Intent into velocity / facing / animation
    //   weapon   -> Intent.fire (+ cooldown) spawns bullets
    //   movement -> integrates velocity (gravity) into position
    //   collision-> corrects penetration, sets FLAG_GROUNDED, kills bullets on walls
    //   combat   -> bullet-vs-actor hits: damage, respawn/destroy (queues destroys)
    //   lifetime -> ticks TTLs, queues expired bullets
    //   flush    -> the ONE safe point that actually destroys queued entities
    //   animator -> advances frames over the now-resolved state
    //   sfx      -> plays everything queued this tick
    aiUpdate(s.world, worldResolve(s.world, s.playerEntity), s.ai);
    inputUpdate(s.world);
    controlUpdate(s.world, s.player, s.playerAnims);
    weaponUpdate(s, dt);
    movementUpdate(s.world, s.phys, dt);
    collisionUpdate(s.world, s.level, s.grid);
    combatUpdate(s);
    lifetimeUpdate(s.world, dt);
    worldFlushDestroys(s.world);
    animatorUpdate(s.world, s.bank, dt);
    sfxFlush(s);
}

void sceneRender(Scene& s, int& outDrawn) {
    BeginMode2D(s.cam);
    levelDraw(s.level, Color{70, 74, 92, 255});
    outDrawn = renderEntities(s.world, s.bank, s.textures, s.cam);
    EndMode2D();
}

void sceneShutdown(Scene& s) {
    texUnloadAll(s.textures);
    sfxUnloadAll(s.sounds);
}

} // namespace sc
