#pragma once
#include <cstdint>

namespace sc {

// Per-entity flags (bitset in a single byte-ish field).
enum EntityFlags : std::uint32_t {
    FLAG_NONE     = 0,
    FLAG_ACTIVE   = 1u << 0, // slot in use
    FLAG_PLAYER   = 1u << 1, // driven by input
    FLAG_GROUNDED = 1u << 2, // resting on a surface this frame
    FLAG_FLIP_X   = 1u << 3, // draw mirrored horizontally
    FLAG_ENEMY    = 1u << 4, // driven by AI, hostile to the player
    FLAG_BULLET   = 1u << 5, // projectile: dies on contact, carries a team
    FLAG_NOGRAVITY= 1u << 6, // movement skips gravity (bullets, flyers)
};

// Entity "kind" is expressed through the flags above. The set FLAG_PLAYER and
// FLAG_ENEMY are the *controllable actors*: Input/AI write their Intent and
// controlUpdate turns it into motion. Bullets are not actors.
inline constexpr std::uint32_t FLAG_ACTOR = FLAG_PLAYER | FLAG_ENEMY;

// Team ids for friendly-fire filtering (team[] in World). 0 = neutral.
enum Team : std::uint8_t {
    TEAM_NEUTRAL = 0,
    TEAM_PLAYER  = 1,
    TEAM_ENEMY   = 2,
};

// Per-entity controller intent: what the controller wants this step, decoupled
// from hardware. Written by Input (keyboard) or AI, consumed by control/weapon.
struct Intent {
    float moveX = 0.0f;   // -1..+1 desired horizontal direction
    float moveY = 0.0f;   // -1..+1 desired vertical direction (flyers / FLAG_NOGRAVITY only)
    bool  jump  = false;  // jump requested this step (edge-triggered upstream)
    bool  fire  = false;  // wants to fire this step (gated by cooldown in weapon)
};

// Selectable enemy brains. The value indexes a function table in AI.cpp, so
// adding a behaviour = append here + add one function + register it. No vtables.
enum AIBehavior : std::uint8_t {
    AI_PATROL = 0, // walk back and forth, turn at walls. Harmless.
    AI_CHASER,     // patrol until the player is near, then close in and shoot.
    AI_SENTRY,     // stationary turret: face the player, fire when in range.
    AI_FLYER,      // ignores gravity, homes toward the player in 2D, shoots.
    AI_COWARD,     // flees when the player gets close.
    AI_COUNT,      // keep last: size of the behaviour table.
};

// Per-entity AI memory. One SoA array; only enemies use it. `home` seeds
// behaviours that anchor to a spot (patrol origin); `timer` is general-purpose
// scratch (cooldowns, state dwell) for behaviours that need it.
struct AIState {
    std::uint8_t behavior = AI_PATROL;
    float        timer    = 0.0f;
    float        home     = 0.0f; // world x the entity was spawned at
};

// Per-entity animation playback state. POD, lives in SoA arrays.
struct AnimState {
    std::uint16_t animId   = 0; // index into AnimationBank's flat table
    std::uint16_t frameIdx = 0; // position within the def's frame list
    float         timer    = 0.0f; // seconds toward next frame
    bool          finished = false; // non-looping anim reached its end
};

// Per-entity procedural sprite distortion ("juice"). A damped spring on the draw
// scale: gameplay events kick scaleX/scaleY away from rest and juiceUpdate (see
// systems/Juice) springs them back, overshooting into a squash/stretch wobble.
// Plain floats (no Vector2) to keep this a leaf header, like the structs above.
struct SpriteFx {
    // Spring state — the squash/stretch. Rest is (1,1) = the native AABB size.
    float scaleX = 1.0f, scaleY = 1.0f; // spring position
    float velX   = 0.0f, velY   = 0.0f; // spring velocity
    // Render output, written each tick by juiceUpdate, read by renderEntities:
    // the spring scale plus continuous breathing/lean, kept separate so those
    // additive effects don't feed back into the spring.
    float drawX  = 1.0f, drawY  = 1.0f; // final non-uniform draw scale
    float lean   = 0.0f;                // final rotation, degrees (velocity tilt)
    float flash  = 0.0f;                // hit-flash intensity, 1 -> 0 (white tint)
};

} // namespace sc
