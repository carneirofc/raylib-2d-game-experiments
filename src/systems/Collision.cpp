#include "systems/Collision.hpp"
#include <cmath>

namespace sc {

void tilemapDraw(const TileMap& m, Color solidColor) {
    for (int cy = 0; cy < m.rows; ++cy)
        for (int cx = 0; cx < m.cols; ++cx)
            if (m.solid[cy * m.cols + cx])
                DrawRectangle(cx * m.tile, cy * m.tile, m.tile, m.tile, solidColor);
}

// Test whether an AABB at (x,y,w,h) overlaps any solid tile.
static bool overlapsSolid(const TileMap& m, float x, float y, float w, float h) {
    const int t = m.tile;
    int x0 = static_cast<int>(std::floor(x / t));
    int y0 = static_cast<int>(std::floor(y / t));
    int x1 = static_cast<int>(std::floor((x + w - 0.001f) / t));
    int y1 = static_cast<int>(std::floor((y + h - 0.001f) / t));
    for (int cy = y0; cy <= y1; ++cy)
        for (int cx = x0; cx <= x1; ++cx)
            if (tileIsSolid(m, cx, cy)) return true;
    return false;
}

void collisionUpdate(World& w, const TileMap& map) {
    const std::size_t n = worldCapacity(w);
    for (std::size_t e = 0; e < n; ++e) {
        if (!(w.flags[e] & FLAG_ACTIVE)) continue;

        Vec2& p = w.pos[e];
        Vec2& v = w.vel[e];
        const Vec2 s = w.size[e];
        const int t = map.tile;

        w.flags[e] &= ~FLAG_GROUNDED;

        // X axis: position already integrated; push out if embedded.
        if (overlapsSolid(map, p.x, p.y, s.x, s.y)) {
            if (v.x > 0.0f) {
                int wallCx = static_cast<int>(std::floor((p.x + s.x) / t));
                p.x = wallCx * static_cast<float>(t) - s.x;
            } else if (v.x < 0.0f) {
                int wallCx = static_cast<int>(std::floor(p.x / t));
                p.x = (wallCx + 1) * static_cast<float>(t);
            }
            v.x = 0.0f;
        }

        // Y axis.
        if (overlapsSolid(map, p.x, p.y, s.x, s.y)) {
            if (v.y > 0.0f) {
                int floorCy = static_cast<int>(std::floor((p.y + s.y) / t));
                p.y = floorCy * static_cast<float>(t) - s.y;
                w.flags[e] |= FLAG_GROUNDED;
            } else if (v.y < 0.0f) {
                int ceilCy = static_cast<int>(std::floor(p.y / t));
                p.y = (ceilCy + 1) * static_cast<float>(t);
            }
            v.y = 0.0f;
        }
    }
}

} // namespace sc
