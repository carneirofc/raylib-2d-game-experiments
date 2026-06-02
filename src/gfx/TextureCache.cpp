#include "gfx/TextureCache.hpp"

namespace sc {

std::uint16_t texLoad(TextureCache& c, const std::string& path) {
    if (auto it = c.byPath.find(path); it != c.byPath.end())
        return it->second;

    Texture2D tex = LoadTexture(path.c_str());
    auto id = static_cast<std::uint16_t>(c.textures.size());
    c.textures.push_back(tex);
    c.byPath.emplace(path, id);
    return id;
}

void texUnloadAll(TextureCache& c) {
    for (auto& t : c.textures)
        if (t.id != 0) UnloadTexture(t);
    c.textures.clear();
    c.byPath.clear();
}

} // namespace sc
