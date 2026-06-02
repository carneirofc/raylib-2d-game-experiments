#pragma once
#include "gfx/AnimationBank.hpp"
#include "gfx/TextureCache.hpp"
#include "world/Components.hpp"

namespace sc {

// Persistent UI state for the sprite/animation editor. Plain data.
struct SpriteEditor {
    int       sheetSel = 0;
    int       animSel  = 0;
    float     previewScale = 4.0f;
    bool      previewPlaying = true;
    AnimState preview{};
    int       lastPreviewAnim = -1;
};

// Renders the editor window if `open`. Lets you re-slice a sheet's grid, edit
// each animation's frames/fps/loop, preview playback, and save back to JSON.
// ImGui frame must be begun already.
void spriteEditorDraw(SpriteEditor& ed, bool& open, AnimationBank& bank, TextureCache& textures);

} // namespace sc
