#pragma once

namespace sc {

// Fixed-timestep accumulator. Plain struct; free functions operate on it.
//
// Usage:
//   timeAdd(clock, GetFrameTime());
//   while (timeConsume(clock)) fixedUpdate(clock.step);
struct FixedTimestep {
    float step       = 1.0f / 60.0f; // seconds per sim tick
    float maxFrame   = 0.25f;        // clamp to avoid spiral-of-death
    float accumulator = 0.0f;
};

// Add real elapsed seconds (clamped).
inline void timeAdd(FixedTimestep& t, float dt) {
    if (dt > t.maxFrame) dt = t.maxFrame;
    t.accumulator += dt;
}

// Consume one fixed step if available.
inline bool timeConsume(FixedTimestep& t) {
    if (t.accumulator >= t.step) {
        t.accumulator -= t.step;
        return true;
    }
    return false;
}

} // namespace sc
