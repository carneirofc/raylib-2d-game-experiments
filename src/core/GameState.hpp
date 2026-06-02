#pragma once
#include <functional>

namespace sc {

// A swappable bundle of per-frame callbacks — the same injection idea as
// AppCallbacks, but one per game "mode" (menu, playing, paused). The composition
// root keeps a STACK of these:
//   * only the TOP state's handleInput/update run (so a pushed Pause freezes the
//     gameplay layer beneath it), but
//   * the WHOLE stack renders bottom-to-top (so Pause draws its overlay over the
//     frozen game).
// Generic: holds std::function only, knows nothing about gameplay. Add a new
// screen (game-over, options, inventory) = build one more GameState and push it.
struct GameState {
    std::function<void()>      handleInput; // top only: read keys, request transitions
    std::function<void(float)> update;      // top only: one fixed sim step
    std::function<void()>      render;       // every layer, bottom-up
    std::function<void()>      onEnter;      // pushed onto the stack
    std::function<void()>      onExit;       // popped off the stack
};

} // namespace sc
