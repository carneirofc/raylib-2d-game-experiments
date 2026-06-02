#include "audio/SoundCache.hpp"

namespace sc {

std::uint16_t sfxLoad(SoundCache& c, const std::string& path) {
    if (auto it = c.byPath.find(path); it != c.byPath.end())
        return it->second;

    Sound s = LoadSound(path.c_str()); // missing file -> empty Sound, logged by raylib
    auto id = static_cast<std::uint16_t>(c.sounds.size());
    c.sounds.push_back(s);
    c.byPath.emplace(path, id);
    return id;
}

void sfxUnloadAll(SoundCache& c) {
    for (auto& s : c.sounds)
        if (s.frameCount > 0) UnloadSound(s);
    c.sounds.clear();
    c.byPath.clear();
}

} // namespace sc
