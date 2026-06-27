# sidescroler

A 2D sprite-based sidescroller written from scratch in **C++23** — no game engine.
Built as a data-oriented (ECS-lite) sandbox aimed at thousands of entities at 60fps.

## What it is

- **Struct-of-arrays world.** Entities are indices; their components live in parallel
  arrays (`pos`, `vel`, `anim`, `intent`, …). No per-entity objects, no inheritance,
  no virtual dispatch in the hot loops.
- **Generational handles.** Stored references use `Entity{index, generation}`; a
  recycled slot can't alias a stale reference (`worldResolve` validates the generation).
- **Dense iteration.** A sparse-set lets systems iterate only the *live* entities, so a
  tick scales with population, not array capacity.
- **Spatial-grid collision.** The level's static solids are bucketed into a uniform grid
  once at load; each entity only tests nearby solids.
- **Data-driven.** Levels (`assets/levels/*.json`) and sprite sheets + animations
  (`assets/sprites/*.json` + `manifest.json`) load from disk — no recompile to tweak.
- **Game feel ("juice").** A per-entity spring (`SpriteFx`) drives squash & stretch,
  a velocity lean, idle breathing, a hit-flash, and camera shake — kicked by gameplay
  events (jump, land, fire, hit) and tunable live. See [Juice](#juice--squash-stretch--game-feel).
- **Game states + menu.** A small state *stack* (menu → play → pause) with a navigable
  main menu and an Options screen; only the top state updates, the whole stack renders.
- **In-game tools.** ImGui debug HUD (F1) and a live sprite/animation editor (F2).

## Stack

- **C++23**, g++ 15.1.0 (MinGW) on Windows
- **raylib 5.5** — window / render / input / audio
- **Dear ImGui 1.92.1** + **rlImGui** (pinned to `main`) — debug UI + sprite editor
- **nlohmann/json 3.11.3** — sprite/level sidecar files
- **CMake 4.x** + FetchContent — all deps fetched at configure time, no manual installs

## Build

```sh
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j
```

Binary: `build/sidescroler.exe`. Assets are copied to `build/assets/` post-build.

> First configure clones raylib / imgui / rlImGui / json — slow once, cached after.

### Build options

| Option | Default | Effect |
|--------|---------|--------|
| `-DSC_ENABLE_DEBUG=ON\|OFF` | `ON` | Compile in the ImGui HUD + sprite editor pass. `OFF` for a release-style build. |
| `-DCMAKE_BUILD_TYPE=Release\|Debug` | `Release` | Standard CMake build type. |

Example release build:
```sh
cmake -S . -B build -G "MinGW Makefiles" -DSC_ENABLE_DEBUG=OFF
cmake --build build -j
```

> See [docs/references.md](docs/references.md) for the exact dependency pins, the
> tag-driven release pipeline, dependency-upgrade notes, and curated references for the
> techniques used here.

## Run

```sh
build/sidescroler.exe        # run from the repo root (assets resolve relative to cwd)
```

- **Move:** ← / → or A / D
- **Jump:** Space / W / ↑
- **Fire:** J / Left-Ctrl (held = autofire)
- **Menu / pause:** Enter confirms, ↑/↓ select, **Esc** pauses in-game / backs out of menus
- **F1:** toggle debug HUD (FPS, entity counts, spawn/despawn, physics + **juice** tuning, inspector)
- **F2:** toggle sprite/animation editor (re-slice grid, edit anims, live preview, save to JSON)

## Layout

| Dir | Responsibility |
|-----|----------------|
| `src/core/`    | `App` (window + main loop + fixed timestep, driven by `AppCallbacks`), `Time`, `Types` (entity handle). Generic — no dependency on the layers below. |
| `src/world/`   | `World` struct-of-arrays (generational handles + sparse-set) and POD `Components`. |
| `src/scene/`   | `Scene` (owns gameplay state + the system schedule), `Level`, `SpatialGrid`, `LevelLoad`, `AssetManifest`, `Spawn`. |
| `src/systems/` | Plain functions over the arrays: `Input`, `AI`, `Movement`, `Collision`, `Combat`/`Lifetime`, `Animator`, `Juice` (squash & stretch), `Render`, `Camera`. |
| `src/gfx/`     | `TextureCache`, `SpriteSheet`, `AnimationDef` / `AnimationBank`. |
| `src/debug/`   | `DebugLayer` (injectable pass), `DebugUI`, `SpriteEditor`. |
| `src/main.cpp` | Composition root — wires `Scene` + `DebugLayer` into the `App` callbacks. |
| `assets/`      | `levels/*.json` (world bounds, solids, spawn, grid) and `sprites/` (PNGs, JSON sidecars, `manifest.json`). |

## Architecture in one breath

`main.cpp` builds a `Scene` and a `DebugLayer`, then hands the `App` five callbacks
(`init`, `onFrameStart`, `fixedUpdate`, `render`, `shutdown`). `App` owns the window and
the fixed-timestep loop and knows nothing about gameplay. Each tick, `sceneFixedUpdate`
runs the systems in a fixed order:

```
ai -> input -> control -> weapon -> movement -> collision -> combat -> lifetime -> animator -> juice -> sfx
```

Systems communicate only through the `World` arrays. The `Intent` component is the seam
between "who decides" (keyboard, AI, network) and "what happens" (locomotion, physics) —
see below.

### Input under a fixed timestep

The sim runs at a fixed 60 Hz, but the render loop runs at the monitor's refresh — so the
fixed-update body runs **0, 1, or several times per rendered frame**. `IsKeyPressed` is an
*edge*: true for exactly one rendered frame. Read it *inside* the fixed step and you drop
presses on frames that run zero steps (common on a 144 Hz display) and repeat them on
catch-up frames — jumps that randomly don't fire or double-fire.

The input layer (`src/systems/Input.*`) is a small, reusable three-stage pattern you can
lift into any fixed-timestep project — redefine the actions, keep the rest:

1. **Bind** — `InputBindings` maps logical **actions** (`ACTION_LEFT`, `ACTION_JUMP`, …) to
   physical keys (several keys per action, rebindable, trivially loadable from JSON). No
   key codes anywhere but here.
2. **Sample** — `inputSample` runs **once per frame** (`onFrameStart`, render rate). The
   *only* hardware read: it records each action's held state and latches edge presses into a
   per-action **buffer**. A press is never missed, whatever the framerate.
3. **Consume** — in the fixed step, `inputStep` ages the buffers (sim time) and `inputUpdate`
   builds the player's `Intent` from pure queries: `inputAxis(LEFT,RIGHT)` for movement,
   `inputHeld(FIRE)`, and a *buffered* jump consumed only when grounded (so a press just
   before landing still fires) and cleared on consume (one press = one jump).

Rule: **sample edge input at frame rate; consume it in the fixed step.** The jump buffer is
just one use of the generic per-action buffer — a dash or attack buffers the same way, and
the F1 HUD's *Input* panel shows every action's live held/buffer state. Adding gamepad =
extend `inputSample` to read the pad into the same `InputState`; nothing downstream changes.

## Juice — squash, stretch & game feel

The classic "juice" effects (squash & stretch, lean, hit-flash, screen shake) are all
driven by **one mechanism**: a damped spring per entity. The `SpriteFx` component holds a
non-uniform draw scale; gameplay events *kick* it away from rest and `juiceUpdate` springs
it back, overshooting into a jelly wobble. One spring + one set of tunables covers every
effect, and overlapping events (land while still firing) blend automatically.

- **Component** (`world/Components.hpp`): `SpriteFx` — spring scale + velocity, plus the
  per-frame render output (`drawX/drawY` scale, `lean`, `flash`). Plain floats, in the SoA
  arrays like every other component.
- **System** (`systems/Juice.cpp`): `juiceUpdate` integrates the spring toward `(1,1)`,
  layers idle breathing + velocity lean onto grounded actors, and fades the flash. The
  spring is semi-implicit Euler — stable at the fixed step, underdamped for the overshoot.
- **Event kicks** live at the source of each event, so the trigger reads naturally:
  | Event | Where | Effect |
  |-------|-------|--------|
  | Jump  | `controlUpdate` (Input.cpp) | tall & thin stretch |
  | Land  | `collisionUpdate` (Collision.cpp) | flat & wide squash, scaled by impact speed |
  | Fire  | `weaponUpdate` (Combat.cpp) | recoil pop |
  | Hit   | `combatUpdate` (Combat.cpp) | squash + white flash + camera shake |
- **Render** (`systems/Render.cpp`): `DrawTexturePro` scales the AABB around a pivot at the
  sprite's **feet** (bottom-center), so a squash plants on the ground and a stretch grows
  upward. Composes with the existing horizontal flip (`FLAG_FLIP_X` → negative `src.width`).
- **Tuning:** every constant lives in `JuiceConfig` (on the `Scene`), editable live in the
  **F1 debug HUD** ("Juice" section, with Squash/Stretch/Flash test buttons), and the in-game
  **Options** menu toggles squash/stretch and camera shake on the fly.

Adding a new effect = add a kick at the event site (`juiceKick`/`juiceFlash`) and, if it
needs a knob, a field in `JuiceConfig`. No new system, no per-entity branching.

## Extending: adding logic / AI to entities

Behavior plugs in by **writing the `Intent` component**, never by touching velocity or
position directly. `controlUpdate` turns intent into motion for *any* controlled entity,
so AI, networking, and replay all share one locomotion path.

1. **Add a flag** in `world/Components.hpp`: `FLAG_AI = 1u << 4`.
2. **Add a system** `systems/Ai.cpp` shaped like `inputUpdate` — loop the dense list, and
   for `FLAG_AI` entities write `intent[e].moveX` / `intent[e].jump` from whatever logic
   you want (chase the player, patrol, flee).
3. **Generalize control** — let `controlUpdate` process `FLAG_PLAYER | FLAG_AI` instead of
   player-only.
4. **Schedule it** in `sceneFixedUpdate`, right after `inputUpdate` (both produce intent
   before `controlUpdate` consumes it).

For **stateful** behavior (patrol → chase → flee, cooldowns), add an `AiState` POD
component to the `World` arrays (mode enum, timers, and an `Entity target` handle — the
generational check means a dead target stops being chased). The AI system then runs a
state machine, still emitting only `Intent`.

Rule of thumb: **AI reads the world and writes only its own `AiState` + `Intent`.**
Everything downstream (movement, collision, render) stays untouched.
