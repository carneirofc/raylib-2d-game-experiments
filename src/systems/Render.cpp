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

// Lerp a colour toward white by amount a (0..1) for the hit-flash. Alpha is left
// untouched so a flashing sprite doesn't change its transparency.
static inline Color flashed(Color c, float a) {
    if (a <= 0.0f) return c;
    if (a > 1.0f)  a = 1.0f;
    auto up = [&](unsigned char ch) {
        return static_cast<unsigned char>(ch + (255 - ch) * a);
    };
    return Color{up(c.r), up(c.g), up(c.b), c.a};
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
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        const Vector2 p = w.pos[e];
        const Vector2 s = w.size[e];
        if (p.x + s.x < left || p.x > right || p.y + s.y < top || p.y > bottom)
            continue; // culled

        const AnimState& st = w.anim[e];
        if (!bankValid(bank, st.animId)) continue;
        const AnimationDef& def = bank.defs[st.animId];
        if (def.frames.empty()) continue;

        const SpriteSheet& sheet = bankSheetForAnim(bank, st.animId);
        int frame = def.frames[st.frameIdx % def.frames.size()];
        Rectangle src = sheetFrameRect(sheet, frame);
        if (w.flags[e] & FLAG_FLIP_X) src.width = -src.width; // mirror horizontally

        // Squash & stretch: scale the AABB by the entity's SpriteFx around a pivot
        // at its feet (bottom-center), so a squash plants on the ground and a
        // stretch grows upward. Lean rotates about the same pivot; flash tints it.
        // Clamp the scale to a small positive floor: a kick amount >= 1 can drive
        // the scale <= 0, and DrawTexturePro flips a negative dest size but NOT the
        // origin we pass — which would un-plant the feet pivot. Keep it positive.
        const SpriteFx& fx = w.fx[e];
        const float dw = s.x * (fx.drawX > 0.02f ? fx.drawX : 0.02f);
        const float dh = s.y * (fx.drawY > 0.02f ? fx.drawY : 0.02f);
        Rectangle dst{p.x + s.x * 0.5f, p.y + s.y, dw, dh};
        Vector2   origin{dw * 0.5f, dh}; // bottom-center of the scaled sprite
        DrawTexturePro(texGet(textures, sheet.texId), src, dst, origin, fx.lean,
                       flashed(unpack(w.tint[e]), fx.flash));
        ++drawn;
    }
    return drawn;
}

} // namespace sc
