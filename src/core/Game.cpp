#include "core/Game.hpp"
#include "systems/Animator.hpp"
#include "systems/Render.hpp"
#include <rlImGui.h>
#include <imgui.h>
#include <fstream>

namespace sc {

static constexpr const char* kAssetDir  = "assets/sprites";
static constexpr const char* kSheetPng  = "assets/sprites/player.png";
static constexpr const char* kSheetJson = "assets/sprites/player.json";
static constexpr int kFrame = 32;
static constexpr int kCols  = 4;
static constexpr int kRows  = 4;

// --- Generate a placeholder sheet (PNG + JSON) so the demo runs unconfigured.
static void ensurePlaceholderAssets() {
    if (FileExists(kSheetPng) && FileExists(kSheetJson)) return;

    Image img = GenImageColor(kCols * kFrame, kRows * kFrame, BLANK);
    // Each cell: a hue-rotated square plus a marker that shifts per frame, so
    // animation is visibly cycling.
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            int idx = r * kCols + c;
            float h = (idx / static_cast<float>(kCols * kRows)) * 360.0f;
            Color body = ColorFromHSV(h, 0.6f, 0.9f);
            int x0 = c * kFrame, y0 = r * kFrame;
            ImageDrawRectangle(&img, x0 + 4, y0 + 4, kFrame - 8, kFrame - 8, body);
            int mx = x0 + 8 + (idx % 3) * 6;
            ImageDrawRectangle(&img, mx, y0 + 10, 5, 5, RAYWHITE);
        }
    }
    ExportImage(img, kSheetPng);
    UnloadImage(img);

    std::ofstream out(kSheetJson);
    out << R"({
  "texture": "player.png",
  "grid": { "frameW": 32, "frameH": 32, "margin": 0, "spacing": 0 },
  "animations": {
    "idle": { "frames": [0, 1, 2, 3], "fps": 6, "loop": true },
    "run":  { "frames": [4, 5, 6, 7], "fps": 12, "loop": true },
    "jump": { "frames": [8, 9], "fps": 8, "loop": false }
  }
})" << '\n';
    TraceLog(LOG_INFO, "Generated placeholder assets at %s", kSheetPng);
}

static void loadAssets(Game& g) {
    ensurePlaceholderAssets();
    bankLoadSheet(g.bank, "player", kSheetJson, kAssetDir, g.textures);
    g.playerAnims.idle = bankAnimId(g.bank, "player:idle");
    g.playerAnims.run  = bankAnimId(g.bank, "player:run");
    g.playerAnims.jump = bankAnimId(g.bank, "player:jump");
    g.crowdAnimId      = g.playerAnims.idle;
}

static void buildLevel(Game& g) {
    tilemapInit(g.map, 200, 40, kFrame);
    // Floor along the bottom two rows.
    for (int cx = 0; cx < g.map.cols; ++cx) {
        tileSetSolid(g.map, cx, g.map.rows - 1, true);
        tileSetSolid(g.map, cx, g.map.rows - 2, true);
    }
    // A few platforms.
    for (int cx = 10; cx < 18; ++cx) tileSetSolid(g.map, cx, g.map.rows - 6, true);
    for (int cx = 25; cx < 30; ++cx) tileSetSolid(g.map, cx, g.map.rows - 9, true);
    for (int cx = 40; cx < 55; ++cx) tileSetSolid(g.map, cx, g.map.rows - 5, true);
}

static void spawnEntities(Game& g, int n) {
    // Deterministic scatter via index hash (no RNG dependency).
    for (int i = 0; i < n; ++i) {
        EntityId e = worldCreate(g.world);
        unsigned h = static_cast<unsigned>(e) * 2654435761u;
        g.world.pos[e]  = {60.0f + static_cast<float>(h % 5000),
                           40.0f + static_cast<float>((h >> 8) % 300)};
        g.world.vel[e]  = {static_cast<float>(static_cast<int>(h % 120) - 60), 0.0f};
        g.world.size[e] = {static_cast<float>(kFrame), static_cast<float>(kFrame)};
        g.world.tint[e] = 0xFFFFFFFFu;
        if (g.crowdAnimId >= 0) {
            g.world.anim[e].animId   = static_cast<std::uint16_t>(g.crowdAnimId);
            g.world.anim[e].frameIdx = static_cast<std::uint16_t>(h % 4);
        }
    }
}

static void despawnEntities(Game& g, int n) {
    int removed = 0;
    for (std::size_t e = 0; e < worldCapacity(g.world) && removed < n; ++e) {
        if (e == g.playerEntity) continue;
        if (worldAlive(g.world, static_cast<EntityId>(e))) {
            worldDestroy(g.world, static_cast<EntityId>(e));
            ++removed;
        }
    }
}

static void fixedUpdate(Game& g, float dt) {
    inputUpdate(g.world, g.player, g.playerAnims);
    movementUpdate(g.world, g.phys, dt);
    collisionUpdate(g.world, g.map);
    animatorUpdate(g.world, g.bank, dt);
}

static void render(Game& g) {
    BeginDrawing();
    ClearBackground(Color{30, 32, 44, 255});

    BeginMode2D(g.cam);
    tilemapDraw(g.map, Color{70, 74, 92, 255});
    g.dbg.drawnLastFrame = renderEntities(g.world, g.bank, g.textures, g.cam);
    EndMode2D();

    rlImGuiBegin();
    debugHud(g.dbg, g.world, g.phys, g.player);
    spriteEditorDraw(g.editor, g.dbg.showSpriteEditor, g.bank, g.textures);
    rlImGuiEnd();

    EndDrawing();
}

void gameInit(Game& g, int width, int height, const char* title) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title);
    SetTargetFPS(0); // vsync paces; fixed-timestep handles sim
    rlImGuiSetup(true);

    worldInit(g.world, 8192);
    loadAssets(g);
    buildLevel(g);

    // Player entity.
    g.playerEntity = worldCreate(g.world);
    g.world.flags[g.playerEntity] |= FLAG_PLAYER;
    g.world.size[g.playerEntity]  = {static_cast<float>(kFrame), static_cast<float>(kFrame)};
    g.world.pos[g.playerEntity]   = {100.0f, 100.0f};
    if (g.playerAnims.idle >= 0) playAnim(g.world, g.playerEntity, g.playerAnims.idle);

    spawnEntities(g, 1000);

    g.cam.zoom = 1.0f;
    g.cam.offset = {width * 0.5f, height * 0.5f};
}

void gameRun(Game& g) {
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F1)) g.dbg.showHud = !g.dbg.showHud;
        if (IsKeyPressed(KEY_F2)) g.dbg.showSpriteEditor = !g.dbg.showSpriteEditor;

        if (g.dbg.spawnRequest > 0)   { spawnEntities(g, g.dbg.spawnRequest);   g.dbg.spawnRequest = 0; }
        if (g.dbg.despawnRequest > 0) { despawnEntities(g, g.dbg.despawnRequest); g.dbg.despawnRequest = 0; }

        timeAdd(g.clock, GetFrameTime());
        while (timeConsume(g.clock)) fixedUpdate(g, g.clock.step);

        // Camera follows the player.
        if (worldAlive(g.world, g.playerEntity)) {
            g.cam.target = {g.world.pos[g.playerEntity].x, g.world.pos[g.playerEntity].y};
            g.cam.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
        }

        render(g);
    }
}

void gameShutdown(Game& g) {
    rlImGuiShutdown();
    texUnloadAll(g.textures);
    CloseWindow();
}

} // namespace sc
