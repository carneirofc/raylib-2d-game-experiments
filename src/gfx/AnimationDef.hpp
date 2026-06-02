#pragma once
#include <string>
#include <vector>

namespace sc {

// A single named animation: an ordered list of frame indices into a sheet,
// played at `fps`. Plain data.
struct AnimationDef {
    std::string      name;          // e.g. "run"
    std::vector<int> frames;        // indices into the SpriteSheet grid
    float            fps  = 8.0f;
    bool             loop = true;
};

inline float animFrameDuration(const AnimationDef& d) {
    return d.fps > 0.0f ? 1.0f / d.fps : 1.0f;
}
inline int animLength(const AnimationDef& d) {
    return static_cast<int>(d.frames.size());
}

} // namespace sc
