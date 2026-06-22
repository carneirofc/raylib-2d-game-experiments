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

static const char* kActionNames[ACTION_COUNT] = { "left", "right", "jump", "fire" };

void debugHud(DebugState& dbg, World& w, Physics& phys, PlayerConfig& player,
              InputState& input, JuiceConfig& juice) {
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

    ImGui::SeparatorText("Input");
    ImGui::SliderFloat("buffer window", &input.bufferWindow, 0.02f, 0.30f, "%.3f s");
    // Live action state: green = held, the bar = remaining buffer (pending press).
    for (int a = 0; a < ACTION_COUNT; ++a) {
        ImGui::TextColored(input.held[a] ? ImVec4(0.4f, 1.0f, 0.5f, 1.0f)
                                         : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           "%-6s", kActionNames[a]);
        ImGui::SameLine();
        const float frac = input.bufferWindow > 0.0f ? input.buffer[a] / input.bufferWindow : 0.0f;
        ImGui::ProgressBar(frac, ImVec2(120, 0), input.buffer[a] > 0.0f ? "buffered" : "");
    }

    ImGui::SeparatorText("Juice (squash & stretch)");
    EntityIndex sel = worldResolve(w, dbg.inspected); // test buttons act on the inspector target
    ImGui::Checkbox("enabled", &juice.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("camera shake", &juice.shake);
    if (ImGui::TreeNodeEx("spring", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("stiffness", &juice.stiffness, 20.0f, 800.0f);
        ImGui::SliderFloat("damping",   &juice.damping,   0.0f, 60.0f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("kicks")) {
        ImGui::SliderFloat("jump stretch", &juice.jumpStretch, 0.0f, 0.8f);
        ImGui::SliderFloat("land base",    &juice.landBase,    0.0f, 0.8f);
        ImGui::SliderFloat("land /vel",    &juice.landPerVel,  0.0f, 0.002f, "%.4f");
        ImGui::SliderFloat("land max",     &juice.landMax,     0.0f, 0.9f);
        ImGui::SliderFloat("fire punch",   &juice.firePunch,   0.0f, 0.6f);
        ImGui::SliderFloat("hit punch",    &juice.hitPunch,    0.0f, 0.8f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("continuous")) {
        ImGui::SliderFloat("breathe amp",   &juice.breatheAmp,   0.0f, 0.2f);
        ImGui::SliderFloat("breathe speed", &juice.breatheSpeed, 0.0f, 10.0f);
        ImGui::SliderFloat("lean /vel",     &juice.leanPerVel,   0.0f, 0.05f, "%.4f");
        ImGui::SliderFloat("lean max",      &juice.leanMax,      0.0f, 30.0f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("shake")) {
        ImGui::SliderFloat("on hit",     &juice.shakeOnHit,    0.0f, 1.0f);
        ImGui::SliderFloat("decay",      &juice.shakeDecay,    0.1f, 6.0f);
        ImGui::SliderFloat("max offset", &juice.shakeMaxOffset, 0.0f, 40.0f);
        ImGui::TreePop();
    }
    // Live test on the inspected entity (entity id 0 is the player — spawned first).
    ImGui::BeginDisabled(sel == INVALID_INDEX);
    if (ImGui::Button("Squash"))  juiceKick(w.fx[sel], juice.landBase + 0.3f, false);
    ImGui::SameLine();
    if (ImGui::Button("Stretch")) juiceKick(w.fx[sel], juice.jumpStretch, true);
    ImGui::SameLine();
    if (ImGui::Button("Flash"))   juiceFlash(w.fx[sel]);
    ImGui::EndDisabled();
    if (sel == INVALID_INDEX)
        ImGui::TextDisabled("(set an Inspector entity id to test)");

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
        const SpriteFx& fx = w.fx[e];
        ImGui::Text("scale (%.2f, %.2f)  lean %.1f  flash %.2f",
                    fx.drawX, fx.drawY, fx.lean, fx.flash);
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
