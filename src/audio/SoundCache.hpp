#pragma once
#include <raylib.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {

// Loads each sound file once, hands back a stable integer id — the audio twin of
// TextureCache. A missing/failed load is kept as an empty Sound (frameCount 0) so
// ids stay stable and sfxPlay simply no-ops on it (the game runs without assets).
struct SoundCache {
    std::vector<Sound>                             sounds;
    std::unordered_map<std::string, std::uint16_t> byPath;
};

// Returns a stable id for the path, loading on first request. Requires the audio
// device to be initialised (see appRun).
std::uint16_t sfxLoad(SoundCache& c, const std::string& path);
void          sfxUnloadAll(SoundCache& c);

// Play id if it refers to a real, loaded sound; otherwise do nothing.
inline void sfxPlay(const SoundCache& c, std::uint16_t id) {
    if (id < c.sounds.size() && c.sounds[id].frameCount > 0)
        PlaySound(c.sounds[id]);
}

} // namespace sc
