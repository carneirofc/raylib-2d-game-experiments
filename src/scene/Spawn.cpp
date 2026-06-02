#include "scene/Spawn.hpp"
#include "systems/Animator.hpp"   // playAnim

namespace sc {

static constexpr float kEntitySize = 32.0f; // default AABB; matches placeholder frame

Entity spawnPlayer(Scene& s, Vector2 at) {
    Entity e = worldCreate(s.world);
    EntityIndex i = e.index;
    s.world.flags[i] |= FLAG_PLAYER;
    s.world.size[i]   = {kEntitySize, kEntitySize};
    s.world.pos[i]    = at;
    s.world.team[i]   = TEAM_PLAYER;
    s.world.health[i] = static_cast<std::int16_t>(s.combat.playerMaxHealth);
    if (s.playerAnims.idle >= 0) playAnim(s.world, i, s.playerAnims.idle);
    return e;
}

Entity spawnEnemy(Scene& s, Vector2 at, std::uint8_t behavior) {
    Entity e = worldCreate(s.world);
    EntityIndex i = e.index;
    s.world.flags[i] |= FLAG_ENEMY;
    if (behavior == AI_FLYER) s.world.flags[i] |= FLAG_NOGRAVITY;
    s.world.size[i]      = {kEntitySize, kEntitySize};
    s.world.pos[i]       = at;
    s.world.team[i]      = TEAM_ENEMY;
    s.world.health[i]    = static_cast<std::int16_t>(s.combat.enemyHealth);
    s.world.ai[i].behavior = behavior;
    s.world.ai[i].home     = at.x;
    // Flyers tinted bluer, ground enemies reddish — quick visual read.
    s.world.tint[i]   = (behavior == AI_FLYER) ? 0x66CCFFFFu : 0xFF6666FFu;
    if (s.playerAnims.idle >= 0) playAnim(s.world, i, s.playerAnims.idle);
    return e;
}

Entity spawnBullet(Scene& s, EntityIndex shooter, float dir) {
    Entity e = worldCreate(s.world);
    EntityIndex i = e.index;
    const Vector2 sp = s.world.pos[shooter];
    const Vector2 ss = s.world.size[shooter];
    s.world.flags[i] |= FLAG_BULLET | FLAG_NOGRAVITY;
    s.world.size[i]   = {12.0f, 6.0f};
    // Launch from the shooter's centre, nudged forward in the travel direction.
    s.world.pos[i]    = {sp.x + ss.x * 0.5f - 6.0f + dir * s.weapon.muzzleOffset,
                         sp.y + ss.y * 0.5f - 3.0f};
    s.world.vel[i]    = {dir * s.weapon.bulletSpeed, 0.0f};
    s.world.team[i]   = s.world.team[shooter];
    s.world.lifetime[i] = s.weapon.bulletTTL;
    s.world.tint[i]   = (s.world.team[i] == TEAM_PLAYER) ? 0xFFEE55FFu : 0xFF9933FFu;
    if (dir < 0.0f) s.world.flags[i] |= FLAG_FLIP_X;
    if (s.playerAnims.idle >= 0) playAnim(s.world, i, s.playerAnims.idle); // placeholder visual
    return e;
}

void spawnCrowd(Scene& s, int n) {
    for (int k = 0; k < n; ++k) {
        EntityIndex e = worldCreate(s.world).index;
        unsigned h = static_cast<unsigned>(e) * 2654435761u;
        s.world.pos[e]  = {60.0f + static_cast<float>(h % 5000),
                           40.0f + static_cast<float>((h >> 8) % 300)};
        s.world.vel[e]  = {static_cast<float>(static_cast<int>(h % 120) - 60), 0.0f};
        s.world.size[e] = {kEntitySize, kEntitySize};
        s.world.tint[e] = 0xFFFFFFFFu;
        if (s.crowdAnimId >= 0) {
            s.world.anim[e].animId   = static_cast<std::uint16_t>(s.crowdAnimId);
            s.world.anim[e].frameIdx = static_cast<std::uint16_t>(h % 4);
        }
    }
}

void despawnCrowd(Scene& s, int n) {
    EntityIndex playerIdx = worldResolve(s.world, s.playerEntity);
    int removed = 0;
    // Walk the dense list back-to-front: worldDestroy swap-removes the entry,
    // moving the last live entity into slot k — which we've already passed, so
    // each slot is visited exactly once.
    for (std::size_t k = s.world.aliveCount; k-- > 0 && removed < n;) {
        EntityIndex e = s.world.dense[k];
        if (e == playerIdx) continue;
        worldDestroy(s.world, worldHandle(s.world, e));
        ++removed;
    }
}

} // namespace sc
