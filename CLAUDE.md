# sidescroler

2D sprite-based sidescroller from scratch. No game engine.

## Stack
- **C++23**, g++ 15.1.0 (MinGW) on Windows.
- **raylib 5.5** — window/render/input/audio.
- **Dear ImGui 1.91.5** + **rlImGui** — debug UI + sprite/anim editor.
- **nlohmann/json 3.11.3** — sprite/anim sidecar files.
- **CMake 4.x** + FetchContent — deps fetched at configure time, no manual installs.

## Build
```
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j
```
Binary: `build/sidescroler.exe`. Assets copied to `build/assets/` post-build.

First configure clones raylib/imgui/rlImGui/json — slow once, cached after.

## Architecture — data-oriented ECS-lite
- `World` (src/world/) owns **struct-of-arrays**: parallel arrays indexed by `EntityIndex`. No per-entity heap, no polymorphism. Free-list recycles slots; a per-slot **generation** counter invalidates stale handles.
- **Identity:** `EntityIndex` (raw slot) for hot iteration; `Entity{index,gen}` for stored references (player, inspector). `worldResolve(handle)` validates the generation — a recycled slot never aliases a stale reference.
- **Dense iteration:** a sparse-set (`dense`/`denseSlot`) gives systems a contiguous list of live indices, so a tick scales with `aliveCount`, not capacity.
- **Systems** are plain functions over the arrays (src/systems/): Movement, Input, AI, Collision, Combat, Lifetime, Animator, Juice, Render, Camera. The schedule lives in `sceneFixedUpdate` (src/scene/Scene.cpp).
- **Input under the fixed timestep (src/systems/Input.*):** the sim is 60Hz but the render loop runs at refresh, so the fixed-update body runs 0..N times/frame. `IsKeyPressed` is an edge (true one frame), so reading it inside the step drops/repeats presses. A reusable three-stage layer fixes it: **bind** (`InputBindings`: logical `InputAction`s -> keys, rebindable); **sample** (`inputSample`, once per frame in `onFrameStart` — the only hardware read; latches edge presses into a per-action buffer); **consume** (per step: `inputStep` ages buffers in sim time, `inputUpdate` builds player `Intent` via pure queries `inputAxis`/`inputHeld`/`inputBuffered`+`inputConsume`). The jump buffer is one instance of the generic per-action buffer (consumed when grounded, cleared on consume). Rule: sample edges at frame rate, consume in the fixed step. Add an action = extend the enum + bind a key; add a device = extend `inputSample`.
- Built for **thousands of entities at 60fps** — dense array iteration, single texture atlas for draw batching, and a uniform **spatial grid** broad-phase (src/scene/SpatialGrid) that buckets the level's static solids so collision tests only nearby ones.

## Layers — separation of concerns
- `core/` (App, Time, Types) is **generic**: window + main loop + fixed timestep, driven by `AppCallbacks`. No includes of gfx/debug/systems/scene.
- `scene/` owns gameplay state (`Scene`: World, Level, SpatialGrid, Camera, configs, bank) and the system schedule. No ImGui.
- `main.cpp` is the **composition root**: wires Scene + DebugLayer into the App callbacks.
- `debug/DebugLayer` is an injectable pass, rendered separately and guarded by `SC_ENABLE_DEBUG` (CMake option, default ON).

## Sprites & animation (src/gfx/)
- Data-driven: each sheet has a JSON sidecar (`assets/sprites/*.json`) — grid params + named animations (frame list, fps, loop).
- `TextureCache` loads textures once. `SpriteSheet` slices grid → frame rects. `AnimationBank` parses JSON, resolves anim names → int ids once at load (hot loop never hashes strings).
- `Animator` system advances frame indices over the World arrays.
- `SpriteEditor` (F2) — in-game ImGui tool: re-slice grid, edit anims, live preview, save back to JSON.

## Sprite distortion / "juice" (src/systems/Juice.*)
- Image distortion is one mechanism: a **damped spring** per entity. `SpriteFx` (in Components.hpp) holds the spring scale + velocity and the per-frame render output (`drawX/drawY`, `lean`, `flash`). Gameplay events *kick* the spring; `juiceUpdate` springs it back to `(1,1)`, overshooting into a squash/stretch wobble.
- **Kicks at the source:** jump→stretch (controlUpdate), land→squash scaled by impact speed (collisionUpdate, reads `v.y` before collision zeroes it), fire→recoil (weaponUpdate), hit→squash + white flash + camera shake (combatUpdate). Each is one `juiceKick`/`juiceFlash`/`sceneShake` call.
- **Render** scales the AABB about a **feet pivot** (bottom-center) so squash plants and stretch grows up; composes with the `FLAG_FLIP_X` mirror. Idle breathe + velocity lean are layered on grounded actors without feeding back into the spring.
- All constants live in `JuiceConfig` on the `Scene` — live-tunable in the F1 HUD ("Juice" section) and toggled from the in-game Options menu. New effect = add a kick at the event site (+ a `JuiceConfig` field if it needs a knob); no new system.

## Game states & menu (src/core/GameState.hpp, src/main.cpp)
- A **stack** of `GameState` (per-mode callback bundles): only the top state's `handleInput`/`update` run (a pushed Pause freezes the game beneath), but the whole stack renders bottom-up (Pause overlays the frozen game). Stack mutations are deferred via `pending` so they never run mid-iteration.
- The composition root builds menu → play → pause + a navigable main menu (↑/↓/Enter) and an Options screen wired to the live `JuiceConfig`. Menu "Quit" sets a flag read by the generic `AppCallbacks::shouldQuit`.

## Layout
- `src/core/` — App (window + loop + fixed timestep, `AppCallbacks`), Time, Types (Entity handle).
- `src/world/` — World SoA (generational handles + sparse-set) + Components.
- `src/scene/` — Scene (gameplay owner + schedule), Level, SpatialGrid, LevelLoad, AssetManifest, Spawn.
- `src/systems/` — Movement, Input, AI, Collision, Combat, Lifetime, Animator, Juice (squash & stretch), Render, Camera.
- `src/gfx/` — TextureCache, SpriteSheet, AnimationDef/Bank.
- `src/debug/` — DebugLayer (injectable pass), DebugUI, SpriteEditor.
- `src/main.cpp` — composition root.
- `assets/levels/` — level JSON (world bounds, solids, spawn, grid cell size).
- `assets/sprites/` — PNGs + JSON sidecars + `manifest.json` (sheets + role→anim map).

## Build options
- `-DSC_ENABLE_DEBUG=OFF` compiles out the ImGui HUD + sprite editor pass (default ON).
