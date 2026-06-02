#include "systems/Input.hpp"
#include "systems/Animator.hpp"
#include <raylib.h>

namespace sc {

void inputUpdate(World& w, const PlayerConfig& cfg, const PlayerAnims& anims) {
    const bool left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    const bool right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    const bool jump  = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);

    const std::size_t n = worldCapacity(w);
    for (std::size_t e = 0; e < n; ++e) {
        if (!(w.flags[e] & FLAG_ACTIVE) || !(w.flags[e] & FLAG_PLAYER)) continue;

        float vx = 0.0f;
        if (left)  vx -= cfg.moveSpeed;
        if (right) vx += cfg.moveSpeed;
        w.vel[e].x = vx;

        if (vx < 0.0f) w.flags[e] |= FLAG_FLIP_X;
        else if (vx > 0.0f) w.flags[e] &= ~FLAG_FLIP_X;

        const bool grounded = w.flags[e] & FLAG_GROUNDED;
        if (jump && grounded) {
            w.vel[e].y = -cfg.jumpSpeed;
        }

        // Animation selection: jump in air, run when moving, else idle.
        int want;
        if (!grounded)      want = anims.jump;
        else if (vx != 0.0f) want = anims.run;
        else                 want = anims.idle;
        if (want >= 0) playAnim(w, static_cast<EntityId>(e), want);
    }
}

} // namespace sc
