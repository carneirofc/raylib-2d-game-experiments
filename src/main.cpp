#include "core/App.hpp"
#include "core/GameState.hpp"
#include "scene/Scene.hpp"
#include "systems/Camera.hpp"
#ifdef SC_ENABLE_DEBUG
#include "debug/DebugLayer.hpp"
#include <rlImGui.h>
#endif
#include <raylib.h>
#include <functional>
#include <vector>

namespace {
constexpr const char* kManifest = "assets/sprites/manifest.json";
constexpr const char* kLevel    = "assets/levels/level0.json";

// Centered text helper for the menu/pause screens.
void drawCentered(const char* text, int y, int size, Color c) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, y, size, c);
}
} // namespace

int main() {
    sc::Scene scene;
#ifdef SC_ENABLE_DEBUG
    sc::DebugLayer debug;
#endif

    sc::AppConfig cfg{1280, 720, "sidescroler"};
    sc::AppCallbacks cb;

    // ---- State stack. Mutating it mid-callback (while iterating to render, or
    // from inside the running state) is unsafe, so states don't push/pop directly:
    // they set `pending` and the app applies it once at the next frame start. ----
    std::vector<sc::GameState> stack;
    std::function<void()>      pending;
    auto pushState = [&](sc::GameState st) {
        if (st.onEnter) st.onEnter();
        stack.push_back(std::move(st));
    };
    auto popState = [&] {
        if (stack.empty()) return;
        if (stack.back().onExit) stack.back().onExit();
        stack.pop_back();
    };

    // Factories reference one another for transitions, so declare first, assign after.
    std::function<sc::GameState()> makeMenu, makePlay, makePause;

    makeMenu = [&]() -> sc::GameState {
        sc::GameState st;
        st.render = [&] {
            int h = GetScreenHeight();
            drawCentered("SIDESCROLER", h / 2 - 90, 64, RAYWHITE);
            drawCentered("Press ENTER to play", h / 2 + 10, 24, Color{180, 184, 200, 255});
            drawCentered("A/D move    SPACE jump    J fire    ESC pause",
                         h / 2 + 50, 18, Color{120, 124, 140, 255});
        };
        st.handleInput = [&] {
            if (IsKeyPressed(KEY_ENTER))
                pending = [&] { popState(); pushState(makePlay()); };
        };
        return st;
    };

    makePlay = [&]() -> sc::GameState {
        sc::GameState st;
        st.update = [&](float dt) { sc::sceneFixedUpdate(scene, dt); };
        st.render = [&] {
            sc::cameraFollow(scene.cam, scene.world, scene.playerEntity,
                             GetScreenWidth(), GetScreenHeight());
            int drawn = 0;
            sc::sceneRender(scene, drawn);
#ifdef SC_ENABLE_DEBUG
            sc::debugDraw(debug, scene, drawn);
#else
            (void)drawn;
#endif
        };
        st.handleInput = [&] {
            if (IsKeyPressed(KEY_ESCAPE)) pending = [&] { pushState(makePause()); };
        };
        return st;
    };

    makePause = [&]() -> sc::GameState {
        sc::GameState st;
        st.render = [&] {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 140});
            int h = GetScreenHeight();
            drawCentered("PAUSED", h / 2 - 60, 56, RAYWHITE);
            drawCentered("ESC to resume", h / 2 + 10, 24, Color{180, 184, 200, 255});
        };
        st.handleInput = [&] {
            if (IsKeyPressed(KEY_ESCAPE)) pending = [&] { popState(); };
        };
        return st;
    };

    cb.init = [&] {
#ifdef SC_ENABLE_DEBUG
        rlImGuiSetup(true);
#endif
        sc::sceneInit(scene, cfg.width, cfg.height, kManifest, kLevel);
        pushState(makeMenu());
    };

    cb.onFrameStart = [&] {
#ifdef SC_ENABLE_DEBUG
        sc::debugHotkeys(debug);
        sc::debugApplyRequests(debug, scene);
#endif
        if (!stack.empty() && stack.back().handleInput) stack.back().handleInput();
        if (pending) { auto apply = pending; pending = nullptr; apply(); }
    };

    // Only the top state simulates — a pushed Pause has no update, freezing play.
    cb.fixedUpdate = [&](float dt) {
        if (!stack.empty() && stack.back().update) stack.back().update(dt);
    };

    cb.render = [&] {
        BeginDrawing();
        ClearBackground(Color{30, 32, 44, 255});
        for (auto& st : stack)
            if (st.render) st.render(); // bottom-to-top: overlays stack on gameplay
        EndDrawing();
    };

    cb.shutdown = [&] {
        sc::sceneShutdown(scene);
#ifdef SC_ENABLE_DEBUG
        rlImGuiShutdown();
#endif
    };

    sc::appRun(cfg, cb);
    return 0;
}
