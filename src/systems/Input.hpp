#pragma once
#include "world/World.hpp"
#include "gfx/AnimationBank.hpp"
#include "systems/Juice.hpp" // JuiceConfig, juiceKick
#include <cstdint>

namespace sc {

// Animation ids the player uses, resolved once at startup.
struct PlayerAnims {
    int idle = -1;
    int run  = -1;
    int jump = -1;
};

struct PlayerConfig {
    float moveSpeed = 220.0f; // px/s horizontal
    float jumpSpeed = 520.0f; // px/s initial upward
};

// ===========================================================================
// Reusable input layer — a reference pattern for input under a FIXED TIMESTEP.
//
// The trap: the sim runs at a fixed rate but the render loop runs at refresh, so
// the fixed-update body runs 0..N times per rendered frame. `IsKeyPressed` is an
// EDGE (true for one frame); read it inside the step and you DROP presses on
// 0-step frames and REPEAT them on catch-up frames. The fix is three stages:
//
//   1. BIND   — logical actions -> physical keys (data; rebindable).
//   2. SAMPLE — once per frame (render rate): the only hardware read. Records the
//               held level-state and latches edge presses into a per-action
//               buffer that survives until a step consumes it.
//   3. CONSUME— in the fixed step: pure queries over the snapshot. Held axes read
//               directly; edge actions are consumed from the buffer (one press =
//               one action). Buffers decay in SIM time (inputStep).
//
// Porting to another project = redefine InputAction, set bindings, reuse the
// rest. Adding gamepad = extend inputSample to also read the pad into the same
// state; nothing downstream changes.
// ===========================================================================

// Logical actions, decoupled from physical keys. Add one, bind a key, and the
// whole sample/buffer/query pipeline covers it with no other changes.
enum InputAction : std::uint8_t {
    ACTION_LEFT = 0,
    ACTION_RIGHT,
    ACTION_JUMP,
    ACTION_FIRE,
    ACTION_COUNT, // keep last: array sizes / loop bound
};

inline constexpr int MAX_KEYS_PER_ACTION = 3; // alternates, e.g. arrows + WASD

// Action -> physical keys (raylib KEY_* codes; 0 == KEY_NULL == empty slot).
// Plain data: copy and overwrite to rebind, or load from JSON like the other
// config. Build the stock layout with defaultBindings().
struct InputBindings {
    int keys[ACTION_COUNT][MAX_KEYS_PER_ACTION];
};
InputBindings defaultBindings();

// Per-action runtime state, refreshed by inputSample once per frame. `held` is
// safe to read in the fixed step (level state, constant across a frame's substeps);
// edge presses must go through `buffer` (never query a raw edge in the step).
struct InputState {
    bool  held[ACTION_COUNT]   = {}; // any bound key down this frame
    float buffer[ACTION_COUNT] = {}; // seconds an edge press stays pending (0 = none)
    // How long a fresh press stays buffered. Keep it >= one fixed step so a press
    // latched this frame always survives inputStep's aging to reach the consume in
    // the same step (the debug slider floors it at ~0.02s for this reason).
    float bufferWindow = 0.12f;
};

// FRAME RATE: read the keyboard through the bindings, set held[], and re-arm the
// buffer for any action pressed this frame. The ONLY function that touches
// hardware. Call from onFrameStart, never inside the fixed-step loop.
void inputSample(InputState& in, const InputBindings& binds);

// FIXED STEP: age every action buffer by dt (sim time) so stale presses expire.
// Call once at the top of the step, before consuming.
void inputStep(InputState& in, float dt);

// --- queries (pure; safe to call in the fixed step) ---
inline bool  inputHeld(const InputState& in, InputAction a)     { return in.held[a]; }
inline bool  inputBuffered(const InputState& in, InputAction a) { return in.buffer[a] > 0.0f; }
inline void  inputConsume(InputState& in, InputAction a)        { in.buffer[a] = 0.0f; }
// -1 / 0 / +1 from a held pair (e.g. LEFT/RIGHT), the 1-D movement axis.
inline float inputAxis(const InputState& in, InputAction neg, InputAction pos) {
    return (in.held[pos] ? 1.0f : 0.0f) - (in.held[neg] ? 1.0f : 0.0f);
}

// Game-specific: bind this game's actions onto the player's Intent, built on the
// generic queries above. The jump is consumed from its buffer only when grounded
// (so a press just before landing still fires) and cleared on consume (one press
// = one jump, even across catch-up steps). Enemies get Intent from aiUpdate
// instead — control/weapon can't tell the two sources apart.
void inputUpdate(World& w, InputState& in);

// Intent -> effects. Consumes intent[] for every actor (FLAG_PLAYER|FLAG_ENEMY):
// horizontal velocity, jump (only when grounded), facing flip, and animation
// selection. A successful jump kicks the squash/stretch spring (juice). Firing
// is handled separately by weaponUpdate (it needs to spawn).
void controlUpdate(World& w, const PlayerConfig& cfg, const PlayerAnims& anims,
                   const JuiceConfig& juice);

} // namespace sc
