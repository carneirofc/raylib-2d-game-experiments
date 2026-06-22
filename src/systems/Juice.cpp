#include "systems/Juice.hpp"
#include <algorithm> // std::clamp
#include <cmath>     // std::sin, std::fabs

namespace sc {

void juiceKick(SpriteFx& fx, float amount, bool stretchTall) {
    if (stretchTall) { fx.scaleX = 1.0f - amount; fx.scaleY = 1.0f + amount; }
    else             { fx.scaleX = 1.0f + amount; fx.scaleY = 1.0f - amount; }
    fx.velX = 0.0f;
    fx.velY = 0.0f;
}

// One semi-implicit Euler step of a damped spring toward rest (x = 1). Velocity
// is updated from the force first, then position from the new velocity — stable
// at the fixed timestep even when underdamped (the overshoot we want).
static inline float springStep(float x, float& v, float stiffness, float damping, float dt) {
    float force = -stiffness * (x - 1.0f) - damping * v;
    v += force * dt;
    return x + v * dt;
}

void juiceUpdate(World& w, const JuiceConfig& cfg, float dt, float time) {
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        SpriteFx& fx = w.fx[e];

        if (!cfg.enabled) {
            // Master switch off: draw everything undistorted, but keep the spring
            // state coasting to rest so toggling back on doesn't pop.
            fx.scaleX = springStep(fx.scaleX, fx.velX, cfg.stiffness, cfg.damping, dt);
            fx.scaleY = springStep(fx.scaleY, fx.velY, cfg.stiffness, cfg.damping, dt);
            fx.drawX = fx.drawY = 1.0f;
            fx.lean = 0.0f;
            fx.flash = 0.0f;
            continue;
        }

        // Spring the squash/stretch back toward rest.
        fx.scaleX = springStep(fx.scaleX, fx.velX, cfg.stiffness, cfg.damping, dt);
        fx.scaleY = springStep(fx.scaleY, fx.velY, cfg.stiffness, cfg.damping, dt);

        // Fade the hit-flash (normalized 1 -> 0 over flashTime).
        if (fx.flash > 0.0f)
            fx.flash -= (cfg.flashTime > 0.0f) ? dt / cfg.flashTime : 1.0f;

        // Continuous effects layered on top of the spring (they do NOT feed back
        // into it, so they can't be damped away). Actors only.
        float breathe = 0.0f, lean = 0.0f;
        if (w.flags[e] & FLAG_ACTOR) {
            const bool grounded = w.flags[e] & FLAG_GROUNDED;
            if (grounded && std::fabs(w.vel[e].x) < 1.0f)
                breathe = std::sin(time * cfg.breatheSpeed) * cfg.breatheAmp;
            lean = std::clamp(w.vel[e].x * cfg.leanPerVel, -cfg.leanMax, cfg.leanMax);
        }
        // Breathe taller => slightly thinner, same volume-preserving idea.
        fx.drawX = fx.scaleX - breathe;
        fx.drawY = fx.scaleY + breathe;
        fx.lean  = lean;
    }
}

} // namespace sc
