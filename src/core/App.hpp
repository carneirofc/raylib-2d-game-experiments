#pragma once
#include "core/Time.hpp"
#include <functional>

namespace sc {

// Generic application shell: owns the window, the main loop, and the fixed
// timestep. Knows nothing about gameplay, gfx, or debug — those are injected as
// callbacks by the composition root (main.cpp), keeping core/ free of any
// dependency on the layers above it.
struct AppConfig {
    int         width  = 1280;
    int         height = 720;
    const char* title  = "sidescroler";
};

struct AppCallbacks {
    std::function<void()>      init;         // after InitWindow + GL ready (load textures here)
    std::function<void(float)> fixedUpdate;  // one sim tick (dt = fixed step)
    std::function<void()>      onFrameStart; // per-frame, pre-update (hotkeys, requests, camera)
    std::function<void()>      render;       // full frame: BeginDrawing .. EndDrawing
    std::function<void()>      shutdown;     // before CloseWindow
    std::function<bool()>      shouldQuit;   // optional: return true to end the loop (e.g. menu Quit)
};

// Open the window, run init -> loop(onFrameStart, fixedUpdate*, render) -> shutdown.
void appRun(const AppConfig& cfg, const AppCallbacks& cb);

} // namespace sc
