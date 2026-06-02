#include "scene/LevelLoad.hpp"
#include "systems/AI.hpp"   // aiBehaviorFromName
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <fstream>

using nlohmann::json;

namespace sc {

bool levelLoad(Level& out, LevelSpawn& spawn, const char* path) {
    std::ifstream f(path);
    if (!f) {
        TraceLog(LOG_WARNING, "levelLoad: cannot open %s", path);
        return false;
    }
    json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "levelLoad: parse error in %s: %s", path, e.what());
        return false;
    }

    Level lv;
    if (j.contains("world")) {
        lv.worldW = j["world"].value("width", 0.0f);
        lv.worldH = j["world"].value("height", 0.0f);
    }
    if (j.contains("solids")) {
        for (const auto& s : j["solids"]) {
            levelAddSolid(lv, Rectangle{
                s.value("x", 0.0f), s.value("y", 0.0f),
                s.value("w", 0.0f), s.value("h", 0.0f)});
        }
    }

    LevelSpawn sp;
    if (j.contains("spawn")) {
        const auto& s = j["spawn"];
        if (s.contains("player")) {
            sp.player.x = s["player"].value("x", 100.0f);
            sp.player.y = s["player"].value("y", 100.0f);
        }
        sp.crowd = s.value("crowd", 0);
        if (s.contains("enemies")) {
            for (const auto& en : s["enemies"]) {
                EnemySpawn es;
                es.pos      = {en.value("x", 0.0f), en.value("y", 0.0f)};
                es.behavior = aiBehaviorFromName(en.value("type", std::string{}));
                sp.enemies.push_back(es);
            }
        }
    }
    if (j.contains("grid")) sp.cellSize = j["grid"].value("cellSize", 128.0f);

    out   = std::move(lv);
    spawn = sp;
    TraceLog(LOG_INFO, "levelLoad: %s (%zu solids, crowd %d)", path, out.solids.size(), sp.crowd);
    return true;
}

} // namespace sc
