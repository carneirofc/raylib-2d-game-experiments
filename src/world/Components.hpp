#pragma once
#include <cstdint>

namespace sc {

// Per-entity flags (bitset in a single byte-ish field).
enum EntityFlags : std::uint32_t {
    FLAG_NONE    = 0,
    FLAG_ACTIVE  = 1u << 0, // slot in use
    FLAG_PLAYER  = 1u << 1, // driven by input
    FLAG_GROUNDED= 1u << 2, // resting on a surface this frame
    FLAG_FLIP_X  = 1u << 3, // draw mirrored horizontally
};

// Per-entity animation playback state. POD, lives in SoA arrays.
struct AnimState {
    std::uint16_t animId   = 0; // index into AnimationBank's flat table
    std::uint16_t frameIdx = 0; // position within the def's frame list
    float         timer    = 0.0f; // seconds toward next frame
    bool          finished = false; // non-looping anim reached its end
};

} // namespace sc
