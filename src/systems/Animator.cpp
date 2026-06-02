#include "systems/Animator.hpp"

namespace sc {

void stepAnim(AnimState& st, const AnimationDef& def, float dt) {
    int len = animLength(def);
    if (len <= 0) return;
    if (st.finished && !def.loop) return;

    st.timer += dt;
    float dur = animFrameDuration(def);
    while (st.timer >= dur) {
        st.timer -= dur;
        if (st.frameIdx + 1 < len) {
            ++st.frameIdx;
        } else if (def.loop) {
            st.frameIdx = 0;
        } else {
            st.frameIdx = static_cast<std::uint16_t>(len - 1);
            st.finished = true;
            break;
        }
    }
}

void animatorUpdate(World& w, const AnimationBank& bank, float dt) {
    const std::size_t n = worldCapacity(w);
    for (std::size_t e = 0; e < n; ++e) {
        if (!(w.flags[e] & FLAG_ACTIVE)) continue;
        AnimState& st = w.anim[e];
        if (!bankValid(bank, st.animId)) continue;
        stepAnim(st, bank.defs[st.animId], dt);
    }
}

void playAnim(World& w, EntityId e, int animId) {
    if (!worldAlive(w, e) || animId < 0) return;
    AnimState& st = w.anim[e];
    if (st.animId == animId) return; // already playing
    st.animId   = static_cast<std::uint16_t>(animId);
    st.frameIdx = 0;
    st.timer    = 0.0f;
    st.finished = false;
}

} // namespace sc
