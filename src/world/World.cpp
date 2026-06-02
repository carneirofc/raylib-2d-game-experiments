#include "world/World.hpp"

namespace sc {

static void worldGrow(World& w, std::size_t n) {
    w.flags.resize(n, FLAG_NONE);
    w.pos.resize(n);
    w.vel.resize(n);
    w.size.resize(n, Vec2{16.0f, 16.0f});
    w.sheet.resize(n, 0);
    w.anim.resize(n);
    w.tint.resize(n, 0xFFFFFFFFu);
}

void worldInit(World& w, std::size_t reserve) {
    w.flags.reserve(reserve);
    w.pos.reserve(reserve);
    w.vel.reserve(reserve);
    w.size.reserve(reserve);
    w.sheet.reserve(reserve);
    w.anim.reserve(reserve);
    w.tint.reserve(reserve);
    w.freeList.reserve(reserve);
}

EntityId worldCreate(World& w) {
    EntityId e;
    if (!w.freeList.empty()) {
        e = w.freeList.back();
        w.freeList.pop_back();
    } else {
        e = static_cast<EntityId>(w.flags.size());
        worldGrow(w, w.flags.size() + 1);
    }
    w.flags[e] = FLAG_ACTIVE;
    w.pos[e]   = {};
    w.vel[e]   = {};
    w.size[e]  = {16.0f, 16.0f};
    w.sheet[e] = 0;
    w.anim[e]  = {};
    w.tint[e]  = 0xFFFFFFFFu;
    ++w.aliveCount;
    return e;
}

void worldDestroy(World& w, EntityId e) {
    if (!worldAlive(w, e)) return;
    w.flags[e] = FLAG_NONE;
    w.freeList.push_back(e);
    --w.aliveCount;
}

} // namespace sc
