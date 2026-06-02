#include "systems/Input.hpp"
#include "systems/Animator.hpp"
#include <raylib.h>

namespace sc {

void inputUpdate(World& w) {
    const bool left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    const bool right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    const bool jump  = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    const bool fire  = IsKeyDown(KEY_J) || IsKeyDown(KEY_LEFT_CONTROL); // held = autofire (weapon gates cadence)

    float moveX = 0.0f;
    if (left)  moveX -= 1.0f;
    if (right) moveX += 1.0f;

    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        if (!(w.flags[e] & FLAG_PLAYER)) continue;
        w.intent[e].moveX = moveX;
        w.intent[e].jump  = jump;
        w.intent[e].fire  = fire;
    }
}

void controlUpdate(World& w, const PlayerConfig& cfg, const PlayerAnims& anims) {
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        if (!(w.flags[e] & FLAG_ACTOR)) continue; // players AND ai-driven enemies
        const Intent& in = w.intent[e];

        const float vx = in.moveX * cfg.moveSpeed;
        w.vel[e].x = vx;

        if (vx < 0.0f) w.flags[e] |= FLAG_FLIP_X;
        else if (vx > 0.0f) w.flags[e] &= ~FLAG_FLIP_X;

        const bool grounded = w.flags[e] & FLAG_GROUNDED;
        if (w.flags[e] & FLAG_NOGRAVITY) {
            // Flyers steer vertically too (movement applies no gravity to them).
            w.vel[e].y = in.moveY * cfg.moveSpeed;
        } else if (in.jump && grounded) {
            w.vel[e].y = -cfg.jumpSpeed;
        }

        // Animation selection: jump in air, run when moving, else idle.
        int want;
        if (!grounded)      want = anims.jump;
        else if (vx != 0.0f) want = anims.run;
        else                 want = anims.idle;
        if (want >= 0) playAnim(w, e, want);
    }
}

} // namespace sc
