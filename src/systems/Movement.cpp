#include "systems/Movement.hpp"

namespace sc {

void movementUpdate(World& w, const Physics& phys, float dt) {
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        Vector2& v = w.vel[e];
        if (!(w.flags[e] & FLAG_NOGRAVITY)) {
            v.y += phys.gravity * dt;
            if (v.y > phys.maxFall) v.y = phys.maxFall;
        }
        w.pos[e].x += v.x * dt;
        w.pos[e].y += v.y * dt;
    }
}

} // namespace sc
