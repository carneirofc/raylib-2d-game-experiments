# References & Further Reading

Supplementary notes that don't belong in the [README](../README.md) (overview, build,
run, architecture) or [CLAUDE.md](../CLAUDE.md) (contributor/agent guide): the exact
dependency pins, the release pipeline, how to upgrade a dependency, and curated external
links for the techniques this project leans on.

## Project documentation

| Doc | What's in it |
|-----|--------------|
| [README.md](../README.md) | Project overview, build/run, controls, layout, architecture deep-dives (input layer, juice, extending AI). |
| [CLAUDE.md](../CLAUDE.md) | Contributor/agent guide — conventions and the data-oriented design rules. |
| **docs/references.md** (this page) | Dependency pins, release pipeline, upgrade notes, external references. |

## Dependencies (pinned)

All deps are fetched at configure time via CMake [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html) — there are no vendored sources and no manual installs. Pins live in [CMakeLists.txt](../CMakeLists.txt). The **resolved version** column is what a clean configure actually checks out (read from the `RAYLIB_VERSION` / `IMGUI_VERSION` / `NLOHMANN_JSON_VERSION_*` macros and `git describe` under `build/_deps/`).

| Library | Pin (`GIT_TAG`) | Resolved version | Role | Source · reference |
|---------|-----------------|------------------|------|--------------------|
| raylib | `5.5` | **5.5** | Window, rendering, input, audio | [repo](https://github.com/raysan5/raylib) · [home](https://www.raylib.com/) · [cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html) · [examples](https://www.raylib.com/examples.html) |
| Dear ImGui | `v1.92.1` | **1.92.1** (`IMGUI_VERSION_NUM` 19210) | Debug HUD + sprite/animation editor | [repo](https://github.com/ocornut/imgui) · [wiki](https://github.com/ocornut/imgui/wiki) |
| rlImGui | `main` *(moving)* | commit [`ef129d1`](https://github.com/raylib-extras/rlImGui/commit/ef129d1858373b6fe332c45f85a1dfc1421bebae) · 2026‑06‑16 | raylib ↔ ImGui backend glue | [repo](https://github.com/raylib-extras/rlImGui) |
| nlohmann/json | `v3.11.3` | **3.11.3** | Sprite / level JSON sidecars | [repo](https://github.com/nlohmann/json) · [docs](https://json.nlohmann.me/) |

> **rlImGui tracks `main`, not a release tag.** It has no versioned releases, so the pin is
> a moving branch and a fresh configure can pull newer glue than a previous one — the commit
> above is what's resolved today. If a build breaks after a clean checkout, pin that exact
> rlImGui commit in `CMakeLists.txt` (`GIT_TAG ef129d1…`) before bisecting your own code.

## Toolchain

| Tool | Used for | Reference |
|------|----------|-----------|
| C++23 | Language standard (`CMAKE_CXX_STANDARD 23`, extensions off) | [cppreference](https://en.cppreference.com/w/cpp/23) |
| g++ 15.1.0 (MinGW-w64) | Local Windows compiler | [GCC](https://gcc.gnu.org/) · [MinGW-w64](https://www.mingw-w64.org/) |
| CMake ≥ 3.20 | Build configuration + dependency fetch | [docs](https://cmake.org/cmake/help/latest/) |
| Ninja | CI build generator (fast incremental) | [ninja-build.org](https://ninja-build.org/) |
| MSYS2 (MINGW64) | Windows CI toolchain provider | [msys2.org](https://www.msys2.org/) |

Local builds use the **MinGW Makefiles** generator (see README); CI uses **Ninja**.

## Build & release pipeline

The release workflow lives in [.github/workflows/release.yml](../.github/workflows/release.yml).

**Triggers**
- Pushing a tag matching `v*` (e.g. `v0.1.3`).
- Manual `workflow_dispatch` with a `tag` input (re-release without a new tag push).

**Jobs**
- `build-windows` and `build-linux` each run a 2-entry **matrix** that differs *only* in
  `SC_ENABLE_DEBUG`:
  - `release` → `SC_ENABLE_DEBUG=OFF` — the player build (ImGui HUD + sprite editor compiled out).
  - `debug`   → `SC_ENABLE_DEBUG=ON` — the dev build (F1 HUD + F2 editor compiled in).
- `release` downloads every build artifact, zips each one, and attaches them all to a
  GitHub Release via [softprops/action-gh-release](https://github.com/softprops/action-gh-release).

**Outputs** — four assets per release:

| Asset | OS | `SC_ENABLE_DEBUG` |
|-------|----|----|
| `sidescroler-windows-release.zip` | Windows | OFF |
| `sidescroler-windows-debug.zip`   | Windows | ON  |
| `sidescroler-linux-release.zip`   | Linux   | OFF |
| `sidescroler-linux-debug.zip`     | Linux   | ON  |

Windows builds link `-static-libgcc -static-libstdc++` and copy `libwinpthread-1.dll` next
to the binary so the zip runs on a clean machine. Linux builds install the X11/GL/ALSA dev
packages raylib needs.

**Cutting a release** — bump the version, tag, and push the tag (versions follow
[SemVer](https://semver.org/), commits follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/)):

```sh
git tag -a v0.1.4 -m "Release v0.1.4"
git push origin v0.1.4
```

## Upgrading a dependency

1. Edit the `GIT_TAG` for that dep in [CMakeLists.txt](../CMakeLists.txt).
2. Reconfigure — `FetchContent` re-clones at the new tag (first configure is slow, then cached).
3. Build and smoke-test both `SC_ENABLE_DEBUG=ON` and `OFF` (ImGui is only linked when ON).

Keep the version strings in [CLAUDE.md](../CLAUDE.md)/[README.md](../README.md) in sync with
the pins when you bump.

## Further reading

The techniques this codebase is built on, with the canonical sources worth reading.

### Game loop & fixed timestep
- [Fix Your Timestep! — Glenn Fiedler](https://gafferongames.com/post/fix_your_timestep/) — why the sim runs at a fixed 60 Hz independent of render rate (the core of `src/core/App` + the input layer).
- [Integration Basics — Glenn Fiedler](https://gafferongames.com/post/integration_basics/) — semi-implicit Euler, the integrator used for movement and the juice spring.
- [Semi-implicit (symplectic) Euler method](https://en.wikipedia.org/wiki/Semi-implicit_Euler_method) — the specific integrator: stable at a fixed step, used for movement and the `SpriteFx` spring.
- [Game Loop — Game Programming Patterns](https://gameprogrammingpatterns.com/game-loop.html) — the pattern behind `App` + `AppCallbacks`.

### Data-oriented design & ECS
- [Data-Oriented Design and C++ — Mike Acton (CppCon 2014)](https://www.youtube.com/watch?v=rX0ItVEVjHc) — the mindset behind the struct-of-arrays `World`.
- [Data-Oriented Design book — Richard Fabian](https://www.dataorienteddesign.com/dodbook/) — long-form treatment of the same ideas.
- [AoS and SoA](https://en.wikipedia.org/wiki/AoS_and_SoA) — the struct-of-arrays memory layout the `World` uses for cache-friendly iteration.
- [ECS back and forth — skypjack](https://skypjack.github.io/2019-03-07-ecs-baf-part-2/) and [EnTT](https://github.com/skypjack/entt) — the sparse-set technique used for dense live-entity iteration.
- [Handles are the better pointers — Andre Weissflog](https://floooh.github.io/2018/06/17/handles-vs-pointers.html) — the generational-index handle scheme (`Entity{index, gen}` + `worldResolve`).
- [Object Pool — Game Programming Patterns](https://gameprogrammingpatterns.com/object-pool.html) — the free-list slot recycling behind the `World`'s entity allocation.

### Collision & spatial partition
- [Spatial Partition — Game Programming Patterns](https://gameprogrammingpatterns.com/spatial-partition.html) — the uniform-grid bucketing in `src/scene/SpatialGrid`.
- [2D collision detection — MDN](https://developer.mozilla.org/en-US/docs/Games/Techniques/2D_collision_detection) — axis-aligned bounding-box (AABB) overlap tests, the narrow-phase used after the grid.

### Rendering & sprites
- [Texture atlas](https://en.wikipedia.org/wiki/Texture_atlas) — why all frames live in one sheet (`TextureCache` + `SpriteSheet`): one bind, batched draws.
- [raylib examples](https://www.raylib.com/examples.html) — runnable references for `DrawTexturePro`, cameras, and input.

### Game feel & juice
- [Juice it or lose it — Martin Jonasson & Petri Purho](https://www.youtube.com/watch?v=Fy0aCDmgnxg) — the talk that names the squash/stretch/shake effects in `src/systems/Juice`.
- [The Art of Screenshake — Jan Willem Nijman (Vlambeer)](https://www.youtube.com/watch?v=AJdEqssNZ-U) — hit-stop, recoil, and shake.
- [Damped Springs — Ryan Juckett](https://www.ryanjuckett.com/damped-springs/) — the math behind the `SpriteFx` spring that drives every juice effect.
- [Twelve basic principles of animation](https://en.wikipedia.org/wiki/Twelve_basic_principles_of_animation) — "squash and stretch" as an animation principle.

### Input feel
- [Why Does Celeste Feel So Good to Play? — Game Maker's Toolkit](https://www.youtube.com/watch?v=yorTG9at90g) — jump buffering and forgiveness windows, the reason the input layer buffers presses.

### Tooling & conventions
- [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/) · [Semantic Versioning](https://semver.org/)
- [Git tagging](https://git-scm.com/book/en/v2/Git-Basics-Tagging) — annotated tags drive the release pipeline.
- [GitHub Actions docs](https://docs.github.com/en/actions) · [About releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
- [upload-artifact](https://github.com/actions/upload-artifact) · [download-artifact](https://github.com/actions/download-artifact) · [action-gh-release](https://github.com/softprops/action-gh-release)
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
