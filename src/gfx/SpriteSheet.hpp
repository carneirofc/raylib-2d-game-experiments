#pragma once
#include <raylib.h>
#include <cstdint>

namespace sc {

// Grid parameters for slicing a texture atlas into fixed-size frames.
struct GridParams {
    int frameW  = 32;
    int frameH  = 32;
    int margin  = 0; // border around the whole sheet
    int spacing = 0; // gap between adjacent frames
};

// A texture id (into TextureCache) plus the grid that slices it. `cols`/`rows`
// are derived from the grid + texture size by sheetRecompute(); call it after
// changing grid or texture dimensions.
struct SpriteSheet {
    std::uint16_t texId = 0;
    int           texW  = 0;
    int           texH  = 0;
    GridParams    grid{};
    int           cols  = 0; // derived
    int           rows  = 0; // derived
};

inline void sheetRecompute(SpriteSheet& s) {
    int stepX = s.grid.frameW + s.grid.spacing;
    int stepY = s.grid.frameH + s.grid.spacing;
    s.cols = stepX > 0 ? (s.texW - 2 * s.grid.margin + s.grid.spacing) / stepX : 0;
    s.rows = stepY > 0 ? (s.texH - 2 * s.grid.margin + s.grid.spacing) / stepY : 0;
    if (s.cols < 0) s.cols = 0;
    if (s.rows < 0) s.rows = 0;
}

inline int sheetFrameCount(const SpriteSheet& s) { return s.cols * s.rows; }

// Source rect for a frame index (row-major, left-to-right, top-to-bottom).
inline Rectangle sheetFrameRect(const SpriteSheet& s, int index) {
    if (s.cols <= 0) return {0, 0, 0, 0};
    int col = index % s.cols;
    int row = index / s.cols;
    return {
        static_cast<float>(s.grid.margin + col * (s.grid.frameW + s.grid.spacing)),
        static_cast<float>(s.grid.margin + row * (s.grid.frameH + s.grid.spacing)),
        static_cast<float>(s.grid.frameW),
        static_cast<float>(s.grid.frameH),
    };
}

} // namespace sc
