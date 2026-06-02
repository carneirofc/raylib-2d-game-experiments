#pragma once
#include "core/Types.hpp"
#include "world/Components.hpp"
#include <vector>
#include <cstdint>

namespace sc {

// Struct-of-arrays entity store. Every array is indexed by EntityId and stays
// parallel. Slots are recycled via a free-list. Plain data — systems read and
// write the arrays directly; the functions below just manage allocation.
struct World {
    std::vector<std::uint32_t> flags;   // EntityFlags bitset
    std::vector<Vec2>          pos;
    std::vector<Vec2>          vel;
    std::vector<Vec2>          size;     // full w/h of the AABB
    std::vector<std::uint16_t> sheet;    // index into the sheet/texture table
    std::vector<AnimState>     anim;
    std::vector<std::uint32_t> tint;     // packed 0xRRGGBBAA

    std::vector<EntityId> freeList;
    std::size_t           aliveCount = 0;
};

void     worldInit(World& w, std::size_t reserve = 4096);
EntityId worldCreate(World& w);
void     worldDestroy(World& w, EntityId e);

inline bool worldAlive(const World& w, EntityId e) {
    return e < w.flags.size() && (w.flags[e] & FLAG_ACTIVE);
}
inline std::size_t worldCapacity(const World& w) { return w.flags.size(); }

} // namespace sc
