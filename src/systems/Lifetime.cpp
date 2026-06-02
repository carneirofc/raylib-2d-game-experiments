#include "systems/Lifetime.hpp"

namespace sc {

void lifetimeUpdate(World& w, float dt) {
    for (std::size_t k = 0; k < w.aliveCount; ++k) {
        const EntityIndex e = w.dense[k];
        if (w.lifetime[e] < 0.0f) continue; // persistent
        w.lifetime[e] -= dt;
        if (w.lifetime[e] <= 0.0f) worldQueueDestroy(w, e);
    }
}

} // namespace sc
