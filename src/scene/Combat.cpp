#include "scene/Combat.hpp"
#include "scene/Spawn.hpp"
#include "systems/Juice.hpp" // juiceKick, juiceFlash
#include <raylib.h>
#include <vector>

namespace sc {

void weaponUpdate(Scene& s, float dt) {
    World& w = s.world;
    // Snapshot the count so bullets spawned this tick aren't re-scanned (they're
    // not actors anyway). Indices stay valid even if worldCreate reallocates.
    const std::size_t n = w.aliveCount;
    for (std::size_t k = 0; k < n; ++k) {
        const EntityIndex e = w.dense[k];
        if (!(w.flags[e] & FLAG_ACTOR)) continue;

        if (w.cooldown[e] > 0.0f) w.cooldown[e] -= dt;
        if (w.intent[e].fire && w.cooldown[e] <= 0.0f) {
            const float dir = (w.flags[e] & FLAG_FLIP_X) ? -1.0f : 1.0f;
            spawnBullet(s, e, dir);
            w.cooldown[e] = s.weapon.fireCooldown;
            juiceKick(w.fx[e], s.juice.firePunch, /*stretchTall=*/false); // recoil pop
            sceneSfx(s, s.sfxShoot);
        }
    }
}

void combatUpdate(Scene& s) {
    World& w = s.world;

    // Partition the live set once: bullets vs. the actors they can hit. Reused
    // scratch (single-threaded) avoids a per-tick allocation.
    static std::vector<EntityIndex> bullets, actors;
    bullets.clear();
    actors.clear();
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        if (w.flags[e] & FLAG_BULLET)     bullets.push_back(e);
        else if (w.flags[e] & FLAG_ACTOR) actors.push_back(e);
    }

    for (EntityIndex b : bullets) {
        const Rectangle bb{w.pos[b].x, w.pos[b].y, w.size[b].x, w.size[b].y};
        for (EntityIndex a : actors) {
            if (w.team[a] == w.team[b]) continue; // no friendly fire
            const Rectangle ab{w.pos[a].x, w.pos[a].y, w.size[a].x, w.size[a].y};
            if (!CheckCollisionRecs(bb, ab)) continue;

            w.health[a] = static_cast<std::int16_t>(w.health[a] - s.combat.bulletDamage);
            juiceKick(w.fx[a], s.juice.hitPunch, /*stretchTall=*/false); // recoil from impact
            juiceFlash(w.fx[a]);                 // white pop
            sceneShake(s, s.juice.shakeOnHit);   // a little camera kick
            worldQueueDestroy(w, b);
            sceneSfx(s, s.sfxHit);

            if (w.health[a] <= 0) {
                if (w.flags[a] & FLAG_PLAYER) {
                    // Never destroy the player (camera target / stored handle):
                    // respawn at the level's spawn point instead.
                    w.health[a] = static_cast<std::int16_t>(s.combat.playerMaxHealth);
                    w.pos[a]    = s.spawn.player;
                    w.vel[a]    = {0.0f, 0.0f};
                } else {
                    worldQueueDestroy(w, a);
                    sceneSfx(s, s.sfxEnemyDie);
                }
            }
            break; // bullet is spent on the first actor it overlaps
        }
    }
}

} // namespace sc
