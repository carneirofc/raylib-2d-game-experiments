#include "debug/DebugLayer.hpp"
#include "scene/Spawn.hpp"
#include <rlImGui.h>
#include <raylib.h>

namespace sc {

void debugHotkeys(DebugLayer& d) {
    if (IsKeyPressed(KEY_F1)) d.dbg.showHud = !d.dbg.showHud;
    if (IsKeyPressed(KEY_F2)) d.dbg.showSpriteEditor = !d.dbg.showSpriteEditor;
}

void debugApplyRequests(DebugLayer& d, Scene& s) {
    if (d.dbg.spawnRequest > 0)   { spawnCrowd(s, d.dbg.spawnRequest);   d.dbg.spawnRequest = 0; }
    if (d.dbg.despawnRequest > 0) { despawnCrowd(s, d.dbg.despawnRequest); d.dbg.despawnRequest = 0; }
    if (d.dbg.enemyRequest > 0) {
        EntityIndex p = worldResolve(s.world, s.playerEntity);
        Vector2 at = (p != INVALID_INDEX) ? Vector2{s.world.pos[p].x + 120.0f, s.world.pos[p].y - 80.0f}
                                          : s.spawn.player;
        for (int k = 0; k < d.dbg.enemyRequest; ++k)
            spawnEnemy(s, at, static_cast<std::uint8_t>(d.dbg.enemyBehavior));
        d.dbg.enemyRequest = 0;
    }
}

void debugDraw(DebugLayer& d, Scene& s, int drawn) {
    d.dbg.drawnLastFrame = drawn;
    rlImGuiBegin();
    debugHud(d.dbg, s.world, s.phys, s.player, s.input, s.juice);
    spriteEditorDraw(d.editor, d.dbg.showSpriteEditor, s.bank, s.textures);
    rlImGuiEnd();
}

} // namespace sc
