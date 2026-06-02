#include "debug/DebugUI.hpp"
#include <imgui.h>
#include <raylib.h>

namespace sc {

void debugHud(DebugState& dbg, World& w, Physics& phys, PlayerConfig& player) {
    if (!dbg.showHud) return;

    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug");

    ImGui::Text("FPS: %d  (%.2f ms)", GetFPS(), GetFrameTime() * 1000.0f);
    ImGui::Text("Entities alive: %zu / cap %zu", w.aliveCount, worldCapacity(w));
    ImGui::Text("Drawn last frame: %d", dbg.drawnLastFrame);

    ImGui::SeparatorText("Spawn");
    static int spawnN = 1000;
    ImGui::SliderInt("count", &spawnN, 1, 5000);
    if (ImGui::Button("Spawn")) dbg.spawnRequest += spawnN;
    ImGui::SameLine();
    if (ImGui::Button("Despawn")) dbg.despawnRequest += spawnN;

    ImGui::SeparatorText("Physics");
    ImGui::SliderFloat("gravity", &phys.gravity, 0.0f, 4000.0f);
    ImGui::SliderFloat("max fall", &phys.maxFall, 100.0f, 4000.0f);

    ImGui::SeparatorText("Player");
    ImGui::SliderFloat("move speed", &player.moveSpeed, 0.0f, 800.0f);
    ImGui::SliderFloat("jump speed", &player.jumpSpeed, 0.0f, 1200.0f);

    ImGui::SeparatorText("Inspector");
    int id = (dbg.inspected == INVALID_ENTITY) ? -1 : static_cast<int>(dbg.inspected);
    if (ImGui::InputInt("entity id", &id)) {
        dbg.inspected = (id < 0) ? INVALID_ENTITY : static_cast<EntityId>(id);
    }
    if (dbg.inspected != INVALID_ENTITY && worldAlive(w, dbg.inspected)) {
        EntityId e = dbg.inspected;
        ImGui::Text("pos (%.1f, %.1f)", w.pos[e].x, w.pos[e].y);
        ImGui::Text("vel (%.1f, %.1f)", w.vel[e].x, w.vel[e].y);
        ImGui::Text("anim id %u  frame %u%s", w.anim[e].animId, w.anim[e].frameIdx,
                    (w.flags[e] & FLAG_GROUNDED) ? "  [grounded]" : "");
    } else if (dbg.inspected != INVALID_ENTITY) {
        ImGui::TextDisabled("(not alive)");
    }

    ImGui::Separator();
    ImGui::Checkbox("Sprite Editor (F2)", &dbg.showSpriteEditor);

    ImGui::End();
}

} // namespace sc
