#pragma once
#include "world/World.hpp"

namespace sc {

// Procedural sprite distortion — the "juice" that makes 2D sprites feel alive:
// squash & stretch, a velocity lean, idle breathing, and a hit-flash. The whole
// system is a damped spring per entity (SpriteFx) that gameplay events "kick"
// away from rest; juiceUpdate springs it back, overshooting into a jelly wobble.
//
// Why a spring and not keyframed scales: events fire at unpredictable times and
// can overlap (land while still firing). A spring blends them automatically and
// always settles to rest, with one set of tunables to dial the whole feel.
struct JuiceConfig {
    bool  enabled = true;   // master switch (Options menu); off = draw undistorted
    bool  shake   = true;   // camera-shake switch (Options menu)

    // Damped spring pulling each sprite's scale back to (1,1). Critical damping is
    // ~2*sqrt(stiffness); the default damping sits below that, so the sprite
    // overshoots and wobbles a couple of times before settling (the jelly look).
    float stiffness = 260.0f;
    float damping   = 18.0f;

    // Event kicks. A kick of k snaps one axis to 1+k and the other to 1-k
    // (volume-preserving), so a tall stretch is also thin and a flat squash is
    // also wide. The spring takes it from there.
    float jumpStretch   = 0.32f;   // leaving the ground: tall + thin
    float landBase      = 0.16f;   // minimum landing squash: flat + wide
    float landPerVel    = 0.0006f; // extra squash per px/s of downward impact
    float landMax       = 0.55f;   // clamp so a long fall can't flatten to nothing
    float landVelMin    = 120.0f;  // ignore impacts gentler than this (walking)
    float firePunch     = 0.14f;   // recoil pulse when a shot goes off
    float hitPunch      = 0.40f;   // pulse when an actor takes damage

    // Idle "breathing": a slow sine added to grounded, still actors so they never
    // freeze perfectly. Amplitude in scale units, speed in rad/s.
    float breatheAmp    = 0.04f;
    float breatheSpeed  = 3.0f;

    // Velocity lean: rotate (degrees) proportional to horizontal speed for a sense
    // of momentum, clamped to leanMax.
    float leanPerVel    = 0.011f;
    float leanMax       = 9.0f;

    // Hit-flash: how long (seconds) a damage pulse tints the sprite toward white.
    float flashTime     = 0.09f;

    // Camera shake: trauma added per hit (0..1), how fast it bleeds off (per s),
    // and the peak offset in pixels at full trauma.
    float shakeOnHit    = 0.35f;
    float shakeDecay    = 1.8f;
    float shakeMaxOffset = 12.0f;
};

// Advance every entity's spring one fixed step: integrate the scale toward rest,
// layer idle breathing + velocity lean onto grounded actors, and fade the flash.
// Writes the render output fields (drawX/drawY/lean/flash) read by renderEntities.
// `time` is a monotonic clock (accumulated sim seconds) seeding the breathe phase.
void juiceUpdate(World& w, const JuiceConfig& cfg, float dt, float time);

// --- Event kicks. Call from whichever system detects the event; each only sets
//     spring state, so they're cheap and order-independent. ---

// Snap one axis to 1+amount and the other to 1-amount (volume-preserving), then
// let the spring recover. stretchTall=true => tall & thin (jumps); false => flat
// & wide (landings, recoil, hits). Setting position (not velocity) gives a crisp,
// repeatable pop regardless of the sprite's current distortion.
void juiceKick(SpriteFx& fx, float amount, bool stretchTall);

// Begin a hit-flash at full intensity; juiceUpdate fades it over flashTime.
inline void juiceFlash(SpriteFx& fx) { fx.flash = 1.0f; }

} // namespace sc
