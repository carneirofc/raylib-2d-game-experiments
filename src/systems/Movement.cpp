#include "systems/Movement.hpp"

namespace sc {

void movementUpdate(World& w, const Physics& phys, float dt) {
    const std::size_t n = worldCapacity(w);
    for (std::size_t e = 0; e < n; ++e) {
        if (!(w.flags[e] & FLAG_ACTIVE)) continue;
        Vec2& v = w.vel[e];
        v.y += phys.gravity * dt;
        if (v.y > phys.maxFall) v.y = phys.maxFall;
        w.pos[e].x += v.x * dt;
        w.pos[e].y += v.y * dt;
    }
}

} // namespace sc
