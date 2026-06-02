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
- `World` (src/world/) owns **struct-of-arrays**: parallel arrays indexed by entity id. No per-entity heap, no polymorphism. Free-list recycles ids.
- **Systems** are plain functions over the arrays (src/systems/): Movement, Input, Collision, Render, Animator.
- Built for **thousands of entities at 60fps** — linear array iteration, single texture atlas for draw batching, spatial grid collision broad-phase.

## Sprites & animation (src/gfx/)
- Data-driven: each sheet has a JSON sidecar (`assets/sprites/*.json`) — grid params + named animations (frame list, fps, loop).
- `TextureCache` loads textures once. `SpriteSheet` slices grid → frame rects. `AnimationBank` parses JSON, resolves anim names → int ids once at load (hot loop never hashes strings).
- `Animator` system advances frame indices over the World arrays.
- `SpriteEditor` (F2) — in-game ImGui tool: re-slice grid, edit anims, live preview, save back to JSON.

## Layout
- `src/core/` — Game loop, Time (fixed timestep), Types.
- `src/world/` — World SoA + Components.
- `src/systems/` — Movement, Input, Collision, Render, Animator, Camera.
- `src/gfx/` — TextureCache, SpriteSheet, AnimationDef/Bank.
- `src/debug/` — DebugUI, SpriteEditor.
- `assets/sprites/` — PNGs + JSON sidecars.
