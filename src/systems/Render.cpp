#include "systems/Render.hpp"

namespace sc {

static inline Color unpack(std::uint32_t c) {
    return Color{
        static_cast<unsigned char>((c >> 24) & 0xFF),
        static_cast<unsigned char>((c >> 16) & 0xFF),
        static_cast<unsigned char>((c >> 8) & 0xFF),
        static_cast<unsigned char>(c & 0xFF),
    };
}

int renderEntities(const World& w,
                   const AnimationBank& bank,
                   const TextureCache& textures,
                   const Camera2D& cam) {
    // Camera view bounds in world space (with a margin for culling).
    const float halfW = (GetScreenWidth() * 0.5f) / cam.zoom;
    const float halfH = (GetScreenHeight() * 0.5f) / cam.zoom;
    const float margin = 64.0f;
    const float left   = cam.target.x - halfW - margin;
    const float right  = cam.target.x + halfW + margin;
    const float top    = cam.target.y - halfH - margin;
    const float bottom = cam.target.y + halfH + margin;

    int drawn = 0;
    const std::size_t n = worldCapacity(w);
    for (std::size_t e = 0; e < n; ++e) {
        if (!(w.flags[e] & FLAG_ACTIVE)) continue;
        const Vec2 p = w.pos[e];
        const Vec2 s = w.size[e];
        if (p.x + s.x < left || p.x > right || p.y + s.y < top || p.y > bottom)
            continue; // culled

        const AnimState& st = w.anim[e];
        if (!bankValid(bank, st.animId)) continue;
        const AnimationDef& def = bank.defs[st.animId];
        if (def.frames.empty()) continue;

        const SpriteSheet& sheet = bankSheetForAnim(bank, st.animId);
        int frame = def.frames[st.frameIdx % def.frames.size()];
        Rectangle src = sheetFrameRect(sheet, frame);
        if (w.flags[e] & FLAG_FLIP_X) src.width = -src.width;

        Rectangle dst{p.x, p.y, s.x, s.y};
        DrawTexturePro(texGet(textures, sheet.texId), src, dst,
                       Vector2{0, 0}, 0.0f, unpack(w.tint[e]));
        ++drawn;
    }
    return drawn;
}

} // namespace sc
