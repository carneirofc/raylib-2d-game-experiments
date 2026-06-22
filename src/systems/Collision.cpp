#include "systems/Collision.hpp"
#include <algorithm> // std::min

namespace sc {

void collisionUpdate(World& w, const Level& lv, const SpatialGrid& grid,
                     const JuiceConfig& juice) {
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];

        Vector2& p = w.pos[e];
        Vector2& v = w.vel[e];
        const Vector2 s = w.size[e];

        // Bullets don't resolve against geometry — they're consumed by it.
        if (w.flags[e] & FLAG_BULLET) {
            const Rectangle bbox{p.x, p.y, s.x, s.y};
            bool hit = false;
            gridQuery(grid, bbox, [&](std::uint32_t si) {
                if (CheckCollisionRecs(bbox, lv.solids[si])) hit = true;
            });
            if (hit) worldQueueDestroy(w, e);
            continue;
        }

        w.flags[e] &= ~FLAG_GROUNDED;

        Rectangle box{p.x, p.y, s.x, s.y};
        // Broad-phase: only the solids whose cells the box overlaps are tested.
        gridQuery(grid, box, [&](std::uint32_t si) {
            const Rectangle& solid = lv.solids[si];
            if (!CheckCollisionRecs(box, solid)) return;

            // Overlap region tells us both penetration depth and direction.
            Rectangle ov = GetCollisionRec(box, solid);
            if (ov.width < ov.height) {
                // Resolve along X (shallower) — push left or right.
                if (box.x < solid.x) p.x -= ov.width; else p.x += ov.width;
                v.x = 0.0f;
            } else {
                // Resolve along Y. Landing on top => grounded.
                if (box.y < solid.y) {
                    p.y -= ov.height;
                    // First floor contact this frame: squash by impact speed. v.y
                    // is the downward speed about to be zeroed; the grounded check
                    // fires the kick once (not every frame while resting/walking).
                    if (!(w.flags[e] & FLAG_GROUNDED) && v.y > juice.landVelMin) {
                        float amt = std::min(juice.landMax,
                                             juice.landBase + v.y * juice.landPerVel);
                        juiceKick(w.fx[e], amt, /*stretchTall=*/false); // flat & wide
                    }
                    w.flags[e] |= FLAG_GROUNDED;
                } else {
                    p.y += ov.height;
                }
                v.y = 0.0f;
            }
            box = {p.x, p.y, s.x, s.y}; // refresh for next solid
        });
    }
}

} // namespace sc
