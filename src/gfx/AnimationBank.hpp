#pragma once
#include "gfx/AnimationDef.hpp"
#include "gfx/SpriteSheet.hpp"
#include "gfx/TextureCache.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {

// Owns sprite sheets and their animations, loaded from JSON sidecars.
//
// Animation names are resolved to a flat integer id ONCE at load time
// ("player:run" -> 3). The hot loop stores and looks up only these ints.
// Plain data; free functions below do the work.
struct AnimationBank {
    std::vector<SpriteSheet>   sheets;
    std::vector<std::string>   sheetName;
    std::vector<std::string>   sheetJson;

    std::vector<AnimationDef>  defs;       // flat across all sheets
    std::vector<std::uint16_t> defSheet;   // defs[i] belongs to sheets[defSheet[i]]

    std::unordered_map<std::string, int>           nameToId;    // "sheet:anim" -> flat id
    std::unordered_map<std::string, std::uint16_t> nameToSheet; // sheet name -> index
};

// Load assets/sprites/<name>.json into a sheet slot (new or reused). The JSON's
// "texture" path is relative to assetDir. Safe to call again to reload.
bool bankLoadSheet(AnimationBank& b, const std::string& name,
                   const std::string& jsonPath, const std::string& assetDir,
                   TextureCache& textures);

// Re-read a previously loaded sheet's JSON from disk (hot iteration).
bool bankReloadSheet(AnimationBank& b, const std::string& name, TextureCache& textures);

// Serialize a sheet (grid + animations) back to its JSON sidecar.
bool bankSaveSheet(const AnimationBank& b, std::uint16_t idx);

// Resolve "sheet:anim" -> flat id, or -1 if unknown.
int bankAnimId(const AnimationBank& b, const std::string& sheetAnim);

// All anim ids belonging to a given sheet index.
std::vector<int> bankAnimsOfSheet(const AnimationBank& b, std::uint16_t idx);

inline bool bankValid(const AnimationBank& b, int id) {
    return id >= 0 && id < static_cast<int>(b.defs.size());
}
inline const SpriteSheet& bankSheetForAnim(const AnimationBank& b, int id) {
    return b.sheets[b.defSheet[id]];
}

} // namespace sc
