#include "systems/Input.hpp"
#include "systems/Animator.hpp"
#include <raylib.h>

namespace sc {

InputBindings defaultBindings() {
    InputBindings b{}; // all slots zero (KEY_NULL = unused)
    b.keys[ACTION_LEFT][0]  = KEY_A;     b.keys[ACTION_LEFT][1]  = KEY_LEFT;
    b.keys[ACTION_RIGHT][0] = KEY_D;     b.keys[ACTION_RIGHT][1] = KEY_RIGHT;
    b.keys[ACTION_JUMP][0]  = KEY_SPACE; b.keys[ACTION_JUMP][1]  = KEY_W; b.keys[ACTION_JUMP][2] = KEY_UP;
    b.keys[ACTION_FIRE][0]  = KEY_J;     b.keys[ACTION_FIRE][1]  = KEY_LEFT_CONTROL;
    return b;
}

void inputSample(InputState& in, const InputBindings& binds) {
    // One pass over the bindings: record held level-state, and re-arm an action's
    // buffer the instant any of its keys goes down. IsKeyPressed gives the edge at
    // frame rate, so a press can never fall between two fixed steps.
    for (int a = 0; a < ACTION_COUNT; ++a) {
        bool down = false, pressed = false;
        for (int k = 0; k < MAX_KEYS_PER_ACTION; ++k) {
            const int key = binds.keys[a][k];
            if (key == 0) continue;               // empty slot
            if (IsKeyDown(key))    down = true;
            if (IsKeyPressed(key)) pressed = true; // edge this frame
        }
        in.held[a] = down;
        if (pressed) in.buffer[a] = in.bufferWindow; // latch the press
    }
}

void inputStep(InputState& in, float dt) {
    // Decay buffers in sim time. Aging only advances on real steps, so on a
    // high-refresh display a press waits (never dropped) until a step runs, then
    // gets its full window.
    for (int a = 0; a < ACTION_COUNT; ++a)
        if (in.buffer[a] > 0.0f) {
            in.buffer[a] -= dt;
            if (in.buffer[a] < 0.0f) in.buffer[a] = 0.0f;
        }
}

void inputUpdate(World& w, InputState& in) {
    const float moveX = inputAxis(in, ACTION_LEFT, ACTION_RIGHT);
    const bool  fire  = inputHeld(in, ACTION_FIRE); // held = autofire (weapon gates cadence)

    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        if (!(w.flags[e] & FLAG_PLAYER)) continue;
        w.intent[e].moveX = moveX;
        w.intent[e].fire  = fire;

        // Consume the buffered jump only when grounded. Grounded is last step's
        // result (collision runs after this) — the same value controlUpdate
        // checks, so they agree. Consuming clears the buffer: one press = one
        // jump, even if a catch-up frame runs several steps while it's pending.
        bool jump = false;
        if (inputBuffered(in, ACTION_JUMP) && (w.flags[e] & FLAG_GROUNDED)) {
            jump = true;
            inputConsume(in, ACTION_JUMP);
        }
        w.intent[e].jump = jump;
    }
}

void controlUpdate(World& w, const PlayerConfig& cfg, const PlayerAnims& anims,
                   const JuiceConfig& juice) {
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
            juiceKick(w.fx[e], juice.jumpStretch, /*stretchTall=*/true); // push off: tall & thin
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
