#include "gfx/AnimationBank.hpp"
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <fstream>

using nlohmann::json;

namespace sc {

static bool bankParseInto(AnimationBank& b, std::uint16_t sheetIdx,
                          const std::string& jsonPath, const std::string& assetDir,
                          TextureCache& textures) {
    std::ifstream f(jsonPath);
    if (!f) {
        TraceLog(LOG_WARNING, "AnimationBank: cannot open %s", jsonPath.c_str());
        return false;
    }
    json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "AnimationBank: parse error in %s: %s", jsonPath.c_str(), e.what());
        return false;
    }

    std::string texFile = j.value("texture", std::string{});
    std::string texPath = assetDir.empty() ? texFile : assetDir + "/" + texFile;
    std::uint16_t texId = texLoad(textures, texPath);
    const Texture2D& tex = texGet(textures, texId);

    SpriteSheet s{};
    s.texId = texId;
    s.texW  = tex.width;
    s.texH  = tex.height;
    if (j.contains("grid")) {
        const auto& g = j["grid"];
        s.grid.frameW  = g.value("frameW", 32);
        s.grid.frameH  = g.value("frameH", 32);
        s.grid.margin  = g.value("margin", 0);
        s.grid.spacing = g.value("spacing", 0);
    }
    sheetRecompute(s);
    b.sheets[sheetIdx] = s;

    // Drop defs previously belonging to this sheet (reload case), then re-add.
    for (std::size_t i = 0; i < b.defs.size();) {
        if (b.defSheet[i] == sheetIdx) {
            b.defs.erase(b.defs.begin() + i);
            b.defSheet.erase(b.defSheet.begin() + i);
        } else {
            ++i;
        }
    }
    // Erasing shifts flat ids, so rebuild the whole name->id map.
    b.nameToId.clear();
    for (std::size_t i = 0; i < b.defs.size(); ++i)
        b.nameToId[b.sheetName[b.defSheet[i]] + ":" + b.defs[i].name] = static_cast<int>(i);

    if (j.contains("animations")) {
        for (auto& [animName, a] : j["animations"].items()) {
            AnimationDef d;
            d.name = animName;
            d.fps  = a.value("fps", 8.0f);
            d.loop = a.value("loop", true);
            if (a.contains("frames"))
                for (auto& fr : a["frames"]) d.frames.push_back(fr.get<int>());

            int id = static_cast<int>(b.defs.size());
            b.defs.push_back(std::move(d));
            b.defSheet.push_back(sheetIdx);
            b.nameToId[b.sheetName[sheetIdx] + ":" + b.defs.back().name] = id;
        }
    }

    TraceLog(LOG_INFO, "AnimationBank: loaded sheet '%s' (%dx%d, %d anims)",
             b.sheetName[sheetIdx].c_str(), tex.width, tex.height,
             (int)bankAnimsOfSheet(b, sheetIdx).size());
    return true;
}

bool bankLoadSheet(AnimationBank& b, const std::string& name,
                   const std::string& jsonPath, const std::string& assetDir,
                   TextureCache& textures) {
    std::uint16_t idx;
    if (auto it = b.nameToSheet.find(name); it != b.nameToSheet.end()) {
        idx = it->second; // reload into existing slot
    } else {
        idx = static_cast<std::uint16_t>(b.sheets.size());
        b.sheets.emplace_back();
        b.sheetName.push_back(name);
        b.sheetJson.push_back(jsonPath);
        b.nameToSheet.emplace(name, idx);
    }
    b.sheetJson[idx] = jsonPath;
    return bankParseInto(b, idx, jsonPath, assetDir, textures);
}

bool bankReloadSheet(AnimationBank& b, const std::string& name, TextureCache& textures) {
    auto it = b.nameToSheet.find(name);
    if (it == b.nameToSheet.end()) return false;
    std::uint16_t idx = it->second;
    std::string jp = b.sheetJson[idx];
    std::string dir;
    if (auto slash = jp.find_last_of("/\\"); slash != std::string::npos)
        dir = jp.substr(0, slash);
    return bankParseInto(b, idx, jp, dir, textures);
}

bool bankSaveSheet(const AnimationBank& b, std::uint16_t idx) {
    if (idx >= b.sheets.size()) return false;
    const SpriteSheet& sh = b.sheets[idx];

    json j;
    {
        std::ifstream in(b.sheetJson[idx]);
        if (in) {
            try { json old; in >> old; j["texture"] = old.value("texture", std::string{}); }
            catch (...) {}
        }
    }
    j["grid"] = {{"frameW", sh.grid.frameW}, {"frameH", sh.grid.frameH},
                 {"margin", sh.grid.margin}, {"spacing", sh.grid.spacing}};

    json anims = json::object();
    for (std::size_t i = 0; i < b.defs.size(); ++i) {
        if (b.defSheet[i] != idx) continue;
        const AnimationDef& d = b.defs[i];
        anims[d.name] = {{"frames", d.frames}, {"fps", d.fps}, {"loop", d.loop}};
    }
    j["animations"] = anims;

    std::ofstream out(b.sheetJson[idx]);
    if (!out) {
        TraceLog(LOG_WARNING, "AnimationBank: cannot write %s", b.sheetJson[idx].c_str());
        return false;
    }
    out << j.dump(2) << '\n';
    TraceLog(LOG_INFO, "AnimationBank: saved %s", b.sheetJson[idx].c_str());
    return true;
}

int bankAnimId(const AnimationBank& b, const std::string& sheetAnim) {
    auto it = b.nameToId.find(sheetAnim);
    return it == b.nameToId.end() ? -1 : it->second;
}

std::vector<int> bankAnimsOfSheet(const AnimationBank& b, std::uint16_t idx) {
    std::vector<int> out;
    for (std::size_t i = 0; i < b.defSheet.size(); ++i)
        if (b.defSheet[i] == idx) out.push_back(static_cast<int>(i));
    return out;
}

} // namespace sc
