#pragma once
#include "world/World.hpp"
#include <raylib.h>
#include <vector>
#include <cstdint>

namespace sc {

// Uniform tile grid of solid/empty cells. An entity only tests the few tiles
// its AABB overlaps, so collision is O(entities), not O(entities * tiles).
// Plain data; free functions below operate on it.
struct TileMap {
    int cols = 0;
    int rows = 0;
    int tile = 32;
    std::vector<std::uint8_t> solid; // cols*rows, row-major
};

inline void tilemapInit(TileMap& m, int cols, int rows, int tileSize) {
    m.cols = cols;
    m.rows = rows;
    m.tile = tileSize;
    m.solid.assign(static_cast<std::size_t>(cols) * rows, 0);
}

inline bool tileInBounds(const TileMap& m, int cx, int cy) {
    return cx >= 0 && cy >= 0 && cx < m.cols && cy < m.rows;
}
inline void tileSetSolid(TileMap& m, int cx, int cy, bool s) {
    if (tileInBounds(m, cx, cy)) m.solid[cy * m.cols + cx] = s ? 1 : 0;
}
inline bool tileIsSolid(const TileMap& m, int cx, int cy) {
    return tileInBounds(m, cx, cy) && m.solid[cy * m.cols + cx];
}

void tilemapDraw(const TileMap& m, Color solidColor);

// Resolves every active entity's AABB against the tilemap, axis-separated so
// entities slide along walls and land cleanly. Sets/clears FLAG_GROUNDED and
// zeros velocity on the axis of contact.
void collisionUpdate(World& w, const TileMap& map);

} // namespace sc
