#pragma once
#include <raylib.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {

// Loads each texture file once, hands back a stable integer id. Plain data.
struct TextureCache {
    std::vector<Texture2D>                         textures;
    std::unordered_map<std::string, std::uint16_t> byPath;
};

// Returns a stable id for the path, loading on first request.
std::uint16_t texLoad(TextureCache& c, const std::string& path);
void          texUnloadAll(TextureCache& c);

inline const Texture2D& texGet(const TextureCache& c, std::uint16_t id) {
    return c.textures[id];
}

} // namespace sc
