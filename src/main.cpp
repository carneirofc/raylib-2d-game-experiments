#include "core/App.hpp"
#include "core/GameState.hpp"
#include "scene/Scene.hpp"
#include "systems/Camera.hpp"
#ifdef SC_ENABLE_DEBUG
#include "debug/DebugLayer.hpp"
#include <rlImGui.h>
#endif
#include <raylib.h>
#include <cstdio>
#include <functional>
#include <memory>
#include <vector>

namespace {
constexpr const char* kManifest = "assets/sprites/manifest.json";
constexpr const char* kLevel    = "assets/levels/level0.json";

// Centered text helper for the menu/pause screens.
void drawCentered(const char* text, int y, int size, Color c) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, y, size, c);
}

// Move a selection index with the arrow/WS keys, wrapping at both ends.
void menuNavigate(int& sel, int count) {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) sel = (sel + 1) % count;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) sel = (sel + count - 1) % count;
}

// Draw a vertical list of menu items, highlighting the selected one with a caret.
void drawMenuItems(const char* const* items, int count, int sel, int topY) {
    for (int i = 0; i < count; ++i) {
        const bool on = (i == sel);
        const Color c = on ? Color{255, 224, 120, 255} : Color{168, 172, 188, 255};
        char line[96];
        std::snprintf(line, sizeof line, "%s  %s", on ? ">" : "  ", items[i]);
        drawCentered(line, topY + i * 46, on ? 30 : 26, c);
    }
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
    bool                       quit = false; // set by the menu's Quit item; ends the loop
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
    std::function<sc::GameState()> makeMenu, makePlay, makePause, makeOptions;

    // Navigable main menu: Up/Down pick an item, Enter confirms. `sel` is shared by
    // value (shared_ptr) into both lambdas so the choice persists across frames.
    makeMenu = [&]() -> sc::GameState {
        sc::GameState st;
        auto sel = std::make_shared<int>(0);
        st.render = [&, sel] {
            int h = GetScreenHeight();
            drawCentered("SIDESCROLER", h / 2 - 150, 64, RAYWHITE);
            static const char* items[] = {"Play", "Options", "Quit"};
            drawMenuItems(items, 3, *sel, h / 2 - 40);
            drawCentered("Up/Down select    Enter confirm",
                         h / 2 + 130, 18, Color{120, 124, 140, 255});
            drawCentered("A/D move    SPACE jump    J fire    ESC pause",
                         h / 2 + 160, 18, Color{120, 124, 140, 255});
        };
        st.handleInput = [&, sel] {
            menuNavigate(*sel, 3);
            if (IsKeyPressed(KEY_ENTER)) {
                switch (*sel) {
                    case 0: pending = [&] { popState(); pushState(makePlay()); }; break;
                    case 1: pending = [&] { pushState(makeOptions()); };          break;
                    case 2: pending = [&] { quit = true; };                       break;
                }
            }
        };
        return st;
    };

    // Options overlay (pushed over the menu): a list of toggles wired straight to
    // the live JuiceConfig, so flipping one changes the running game's feel.
    makeOptions = [&]() -> sc::GameState {
        sc::GameState st;
        auto sel = std::make_shared<int>(0);
        st.render = [&, sel] {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{20, 22, 32, 235});
            int h = GetScreenHeight();
            drawCentered("OPTIONS", h / 2 - 130, 48, RAYWHITE);
            char a[64], b[64];
            std::snprintf(a, sizeof a, "Squash & stretch:  %s", scene.juice.enabled ? "ON" : "OFF");
            std::snprintf(b, sizeof b, "Camera shake:      %s", scene.juice.shake   ? "ON" : "OFF");
            const char* items[] = {a, b, "Back"};
            drawMenuItems(items, 3, *sel, h / 2 - 30);
            drawCentered("Up/Down move    Enter/Left/Right toggle    ESC back",
                         h / 2 + 120, 18, Color{120, 124, 140, 255});
        };
        st.handleInput = [&, sel] {
            menuNavigate(*sel, 3);
            const bool toggle = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT);
            if (*sel == 0 && toggle) scene.juice.enabled = !scene.juice.enabled;
            else if (*sel == 1 && toggle) scene.juice.shake = !scene.juice.shake;
            else if (*sel == 2 && IsKeyPressed(KEY_ENTER)) pending = [&] { popState(); };
            if (IsKeyPressed(KEY_ESCAPE)) pending = [&] { popState(); };
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
            // Sample the keyboard ONCE per frame (here, at render rate) — the fixed
            // step consumes the snapshot, so presses are never dropped or doubled.
            sc::inputSample(scene.input, scene.binds);
            if (IsKeyPressed(KEY_ESCAPE)) pending = [&] { pushState(makePause()); };
        };
        return st;
    };

    makePause = [&]() -> sc::GameState {
        sc::GameState st;
        // Drop any buffered jump so it can't fire the instant play resumes.
        st.onEnter = [&] { scene.input.buffer[sc::ACTION_JUMP] = 0.0f; };
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

    cb.shouldQuit = [&] { return quit; }; // menu Quit ends the main loop

    sc::appRun(cfg, cb);
    return 0;
}
