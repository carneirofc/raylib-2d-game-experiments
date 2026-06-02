#include "systems/AI.hpp"
#include <cmath>

namespace sc {

// --- Behaviours -------------------------------------------------------------
// Each writes self's Intent for this tick (already zeroed by aiUpdate). They may
// also set FLAG_FLIP_X directly to aim; controlUpdate only overrides facing when
// the entity is actually moving (vx != 0), so a stationary aim sticks.

// Walk in the current facing; turn around when a wall stalls us (grounded but
// horizontal velocity ~0 after last tick's collision). Anchors to AIState.home.
static void aiPatrol(World& w, EntityIndex e, EntityIndex, const AIConfig&) {
    if ((w.flags[e] & FLAG_GROUNDED) && std::fabs(w.vel[e].x) < 1.0f)
        w.flags[e] ^= FLAG_FLIP_X;
    w.intent[e].moveX = (w.flags[e] & FLAG_FLIP_X) ? -1.0f : 1.0f;
}

// Patrol until the player is within aggro range, then close in, fire when near,
// and hop toward a player who is above us.
static void aiChaser(World& w, EntityIndex e, EntityIndex player, const AIConfig& cfg) {
    if (player == INVALID_INDEX) { aiPatrol(w, e, player, cfg); return; }
    const float dx = w.pos[player].x - w.pos[e].x;
    if (std::fabs(dx) >= cfg.aggroRange) { aiPatrol(w, e, player, cfg); return; }
    w.intent[e].moveX = (dx > 0.0f) ? 1.0f : -1.0f;
    w.intent[e].fire  = std::fabs(dx) < cfg.fireRange;
    if ((w.flags[e] & FLAG_GROUNDED) && w.pos[player].y < w.pos[e].y - cfg.jumpReach)
        w.intent[e].jump = true;
}

// Stationary turret: never walks, just faces the player and fires in range.
static void aiSentry(World& w, EntityIndex e, EntityIndex player, const AIConfig& cfg) {
    if (player == INVALID_INDEX) return;
    const float dx = w.pos[player].x - w.pos[e].x;
    if (dx < 0.0f) w.flags[e] |= FLAG_FLIP_X; else w.flags[e] &= ~FLAG_FLIP_X;
    w.intent[e].fire = std::fabs(dx) < cfg.fireRange; // moveX stays 0
}

// Gravity-free homing drone: steers toward the player in 2D (controlUpdate maps
// moveX/moveY to velocity for FLAG_NOGRAVITY actors) and fires when lined up.
static void aiFlyer(World& w, EntityIndex e, EntityIndex player, const AIConfig& cfg) {
    if (player == INVALID_INDEX) return;
    const float dx = w.pos[player].x - w.pos[e].x;
    const float dy = w.pos[player].y - w.pos[e].y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < cfg.aggroRange && dist > 1.0f) {
        w.intent[e].moveX = dx / dist; // normalized -> full flySpeed via control
        w.intent[e].moveY = dy / dist;
        w.intent[e].fire  = std::fabs(dx) < cfg.fireRange;
    }
    if (dx < 0.0f) w.flags[e] |= FLAG_FLIP_X; else w.flags[e] &= ~FLAG_FLIP_X;
}

// Runs away from the player when cornered; otherwise patrols.
static void aiCoward(World& w, EntityIndex e, EntityIndex player, const AIConfig& cfg) {
    if (player == INVALID_INDEX) { aiPatrol(w, e, player, cfg); return; }
    const float dx = w.pos[player].x - w.pos[e].x;
    if (std::fabs(dx) < cfg.aggroRange) w.intent[e].moveX = (dx > 0.0f) ? -1.0f : 1.0f;
    else                                aiPatrol(w, e, player, cfg);
}

// Table indexed by AIBehavior. Order MUST match the enum (AI_PATROL..AI_COWARD).
static const AIBehaviorFn kBehaviors[AI_COUNT] = {
    aiPatrol, aiChaser, aiSentry, aiFlyer, aiCoward,
};

// --- Dispatch ---------------------------------------------------------------

void aiUpdate(World& w, EntityIndex player, const AIConfig& cfg) {
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        if (!(w.flags[e] & FLAG_ENEMY)) continue;
        w.intent[e] = Intent{};
        const std::uint8_t b = w.ai[e].behavior;
        if (b < AI_COUNT) kBehaviors[b](w, e, player, cfg);
    }
}

std::uint8_t aiBehaviorFromName(const std::string& name) {
    if (name == "patrol") return AI_PATROL;
    if (name == "sentry") return AI_SENTRY;
    if (name == "flyer")  return AI_FLYER;
    if (name == "coward") return AI_COWARD;
    return AI_CHASER; // default (also covers "chaser" and unknown)
}

} // namespace sc
