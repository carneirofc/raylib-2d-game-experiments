#include "debug/SpriteEditor.hpp"
#include "systems/Animator.hpp"
#include <rlImGui.h>
#include <imgui.h>
#include <raylib.h>
#include <algorithm>

namespace sc {

void spriteEditorDraw(SpriteEditor& ed, bool& open, AnimationBank& bank, TextureCache& textures) {
    if (!open) return;

    ImGui::SetNextWindowSize(ImVec2(560, 640), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sprite Editor", &open)) { ImGui::End(); return; }

    if (bank.sheets.empty()) {
        ImGui::TextDisabled("No sheets loaded.");
        ImGui::End();
        return;
    }

    // --- Sheet selector ---
    if (ed.sheetSel >= static_cast<int>(bank.sheets.size())) ed.sheetSel = 0;
    if (ImGui::BeginCombo("Sheet", bank.sheetName[ed.sheetSel].c_str())) {
        for (int i = 0; i < static_cast<int>(bank.sheets.size()); ++i) {
            if (ImGui::Selectable(bank.sheetName[i].c_str(), i == ed.sheetSel)) {
                ed.sheetSel = i;
                ed.animSel = 0;
                ed.lastPreviewAnim = -1;
            }
        }
        ImGui::EndCombo();
    }
    auto sheetIdx = static_cast<std::uint16_t>(ed.sheetSel);
    SpriteSheet& sheet = bank.sheets[sheetIdx];

    // --- Grid params (live re-slice) ---
    ImGui::SeparatorText("Grid");
    bool gridChanged = false;
    gridChanged |= ImGui::InputInt("frameW", &sheet.grid.frameW);
    gridChanged |= ImGui::InputInt("frameH", &sheet.grid.frameH);
    gridChanged |= ImGui::InputInt("margin", &sheet.grid.margin);
    gridChanged |= ImGui::InputInt("spacing", &sheet.grid.spacing);
    if (gridChanged) {
        sheet.grid.frameW = std::max(1, sheet.grid.frameW);
        sheet.grid.frameH = std::max(1, sheet.grid.frameH);
        sheetRecompute(sheet);
    }
    ImGui::Text("Grid: %d cols x %d rows (%d frames)", sheet.cols, sheet.rows, sheetFrameCount(sheet));

    // --- Sheet image with grid overlay ---
    ImGui::SeparatorText("Sheet");
    const Texture2D& tex = texGet(textures, sheet.texId);
    ImVec2 imgPos = ImGui::GetCursorScreenPos();
    rlImGuiImage(&tex);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 lineCol = IM_COL32(0, 255, 0, 120);
    for (int c = 0; c <= sheet.cols; ++c) {
        float x = imgPos.x + sheet.grid.margin + c * (sheet.grid.frameW + sheet.grid.spacing);
        dl->AddLine(ImVec2(x, imgPos.y), ImVec2(x, imgPos.y + tex.height), lineCol);
    }
    for (int r = 0; r <= sheet.rows; ++r) {
        float y = imgPos.y + sheet.grid.margin + r * (sheet.grid.frameH + sheet.grid.spacing);
        dl->AddLine(ImVec2(imgPos.x, y), ImVec2(imgPos.x + tex.width, y), lineCol);
    }

    // --- Animation editor ---
    ImGui::SeparatorText("Animations");
    std::vector<int> anims = bankAnimsOfSheet(bank, sheetIdx);
    if (!anims.empty()) {
        if (ed.animSel >= static_cast<int>(anims.size())) ed.animSel = 0;
        if (ImGui::BeginCombo("Animation", bank.defs[anims[ed.animSel]].name.c_str())) {
            for (int i = 0; i < static_cast<int>(anims.size()); ++i) {
                if (ImGui::Selectable(bank.defs[anims[i]].name.c_str(), i == ed.animSel)) {
                    ed.animSel = i;
                    ed.lastPreviewAnim = -1;
                }
            }
            ImGui::EndCombo();
        }

        int animId = anims[ed.animSel];
        AnimationDef& d = bank.defs[animId];

        ImGui::SliderFloat("fps", &d.fps, 1.0f, 30.0f);
        ImGui::Checkbox("loop", &d.loop);

        // Frame list editor.
        ImGui::Text("Frames (%d):", static_cast<int>(d.frames.size()));
        for (std::size_t i = 0; i < d.frames.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::SetNextItemWidth(80);
            ImGui::InputInt("##f", &d.frames[i]);
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) {
                d.frames.erase(d.frames.begin() + i);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
            if ((i + 1) % 4 != 0) ImGui::SameLine();
        }
        if (ImGui::Button("+ frame")) d.frames.push_back(0);

        // --- Live preview (uses the same stepAnim as the runtime) ---
        ImGui::SeparatorText("Preview");
        if (ed.lastPreviewAnim != animId) {
            ed.preview = {};
            ed.preview.animId = static_cast<std::uint16_t>(animId);
            ed.lastPreviewAnim = animId;
        }
        ImGui::SliderFloat("scale", &ed.previewScale, 1.0f, 8.0f);
        ImGui::Checkbox("play", &ed.previewPlaying);
        ImGui::SameLine();
        if (ImGui::Button("step") && !d.frames.empty())
            ed.preview.frameIdx = static_cast<std::uint16_t>((ed.preview.frameIdx + 1) % d.frames.size());
        if (ed.previewPlaying && !d.frames.empty())
            stepAnim(ed.preview, d, GetFrameTime());

        if (!d.frames.empty()) {
            int frame = d.frames[ed.preview.frameIdx % d.frames.size()];
            Rectangle src = sheetFrameRect(sheet, frame);
            ImVec2 sz(sheet.grid.frameW * ed.previewScale, sheet.grid.frameH * ed.previewScale);
            ImVec2 uv0(src.x / tex.width, src.y / tex.height);
            ImVec2 uv1((src.x + src.width) / tex.width, (src.y + src.height) / tex.height);
            ImGui::Image((ImTextureID)(intptr_t)tex.id, sz, uv0, uv1);
            ImGui::Text("frame idx %u -> sheet frame %d", ed.preview.frameIdx, frame);
        }
    } else {
        ImGui::TextDisabled("(this sheet has no animations)");
    }

    // --- Save / Reload ---
    ImGui::SeparatorText("File");
    if (ImGui::Button("Save JSON")) bankSaveSheet(bank, sheetIdx);
    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        bankReloadSheet(bank, bank.sheetName[sheetIdx], textures);
        ed.lastPreviewAnim = -1;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", bank.sheetJson[sheetIdx].c_str());

    ImGui::End();
}

} // namespace sc
