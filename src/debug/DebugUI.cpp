#include "debug/DebugUI.hpp"
#include <imgui.h>
#include <raylib.h>

namespace sc {

static const char* kBehaviorNames[AI_COUNT] = {
    "patrol", "chaser", "sentry", "flyer", "coward",
};
static const char* behaviorName(std::uint8_t b) {
    return b < AI_COUNT ? kBehaviorNames[b] : "?";
}

void debugHud(DebugState& dbg, World& w, Physics& phys, PlayerConfig& player) {
    if (!dbg.showHud) return;

    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug");

    ImGui::Text("FPS: %d  (%.2f ms)", GetFPS(), GetFrameTime() * 1000.0f);
    ImGui::Text("Entities alive: %zu / cap %zu", w.aliveCount, worldCapacity(w));
    ImGui::Text("Drawn last frame: %d", dbg.drawnLastFrame);

    ImGui::SeparatorText("Crowd");
    static int spawnN = 1000;
    ImGui::SliderInt("count", &spawnN, 1, 5000);
    if (ImGui::Button("Spawn")) dbg.spawnRequest += spawnN;
    ImGui::SameLine();
    if (ImGui::Button("Despawn")) dbg.despawnRequest += spawnN;

    ImGui::SeparatorText("Enemies");
    ImGui::Combo("brain", &dbg.enemyBehavior, kBehaviorNames, AI_COUNT);
    if (ImGui::Button("Spawn enemy near player")) dbg.enemyRequest += 1;

    ImGui::SeparatorText("Physics");
    ImGui::SliderFloat("gravity", &phys.gravity, 0.0f, 4000.0f);
    ImGui::SliderFloat("max fall", &phys.maxFall, 100.0f, 4000.0f);

    ImGui::SeparatorText("Player");
    ImGui::SliderFloat("move speed", &player.moveSpeed, 0.0f, 800.0f);
    ImGui::SliderFloat("jump speed", &player.jumpSpeed, 0.0f, 1200.0f);

    ImGui::SeparatorText("Inspector");
    int id = (dbg.inspected.index == INVALID_INDEX) ? -1 : static_cast<int>(dbg.inspected.index);
    if (ImGui::InputInt("entity id", &id)) {
        // Bind to whatever currently occupies that slot (a fresh handle), so the
        // generation check below catches a slot that is later recycled.
        if (id < 0 || !worldAliveIndex(w, static_cast<EntityIndex>(id)))
            dbg.inspected = INVALID_ENTITY;
        else
            dbg.inspected = worldHandle(w, static_cast<EntityIndex>(id));
    }
    EntityIndex e = worldResolve(w, dbg.inspected);
    if (e != INVALID_INDEX) {
        ImGui::Text("pos (%.1f, %.1f)", w.pos[e].x, w.pos[e].y);
        ImGui::Text("vel (%.1f, %.1f)", w.vel[e].x, w.vel[e].y);
        ImGui::Text("anim id %u  frame %u%s", w.anim[e].animId, w.anim[e].frameIdx,
                    (w.flags[e] & FLAG_GROUNDED) ? "  [grounded]" : "");
        ImGui::Text("hp %d  team %u", w.health[e], w.team[e]);
        if (w.flags[e] & FLAG_ENEMY)
            ImGui::Text("brain: %s", behaviorName(w.ai[e].behavior));
    } else if (dbg.inspected != INVALID_ENTITY) {
        ImGui::TextDisabled("(not alive)");
    }

    ImGui::Separator();
    ImGui::Checkbox("Sprite Editor (F2)", &dbg.showSpriteEditor);

    ImGui::End();
}

} // namespace sc
