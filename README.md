# Dungeon Crawlers

A retro-style dungeon crawler RPG in the vein of *Eye of the Beholder*, built from scratch in C++26 with OpenGL 3.3 and SDL3.

> **Current status**: grid-based movement, turn system, melee combat, items & inventory (pickup/use/drop), combat log, hero HUD.

---

## Features

### Implemented
- **3D renderer**: OpenGL 3.3 core profile, instancing, frustum culling, UBO for view/projection
- **Grid camera**: snap-to-grid with lerp animation (0.2s per step), 90° turns, strafe
- **Dungeon**: procedural grid with walls/floor/ceiling rendered as instanced meshes (textures from files)
- **Turn-based system**: `Exploring` → `TurnWaiting` → `TurnAnimating`, edge-triggered input
- **Combat**: melee attack (Space), d20+atkBonus vs AC, critical hits, monster counter-attack
- **Monsters**: billboard sprites (skeleton, slime), station ay AI (placeholder), block player movement
- **Hero HUD**: HP bar, AC, attack bonus, damage dice
- **Combat Log**: colour-coded entries (hit, miss, critical, death), auto-scroll, clear
- **Monster tooltip**: name, HP bar, AC when facing a monster
- **Death/GameOver**: HP ≤ 0 → GameOver popup, all input locked
- **Items & inventory**: 9 item types (weapons, armor, potions, keys, gold, etc.), pickup (Space), inventory UI (I), use/drop
- **Item drops**: floor billboard sprites with per-type textures (downloaded + checkered fallbacks)
- **Debug tools**: Tab toggles debug camera + ImGui panels (camera, turn system, dungeon info)
- **Cross-platform**: `#ifdef __EMSCRIPTEN__` guards, no `filesystem`, no `thread` on Web
- **Retro FBO**: low-resolution framebuffer (`GL_NEAREST`) with configurable resolution

### In Progress
- Save/load system (Phase 1.7)

### Planned
See [ROADMAP.MD](./ROADMAP.MD) for full development plan.

---

## Controls

| Key | Action |
|-----|--------|
| W / S | Move forward / backward |
| A / D | Turn left / right (90°) |
| Q / E | Strafe left / right |
| Space | Attack monster in front / pick up item on tile (context-sensitive) |
| I | Toggle inventory |
| Tab | Toggle debug mode / free camera |

---

## Project Structure

```
src/
├── core/                    # Low-level utilities
│   ├── aabb.h               # Axis-aligned bounding box
│   ├── collision.h          # AABB/Sphere/Ray intersection
│   ├── event_bus.h          # Type-safe publish/subscribe
│   ├── exception.h          # EngineException with source_location
│   ├── logger.h/.cpp        # Thread-safe logging
│   ├── profiler.h/.cpp      # Scoped profiling
│   ├── scoped_sdl.h         # RAII SDL_Init/Quit
│   └── transform.h          # Hierarchy transform with dirty flag
├── engine/                  # Engine layer
│   ├── audio_system.h/.cpp  # WAV/OGG playback (skeleton)
│   ├── delta_time.h/.cpp    # Variable timestep
│   ├── game_state.h/.cpp    # State machine (push/pop/replace)
│   ├── gl_context.h/.cpp    # OpenGL context + glad loader
│   ├── input_manager.h/.cpp # Key/mouse state, action bindings
│   ├── resource_manager.h/.cpp # Resource cache (shaders, meshes, textures)
│   ├── window.h/.cpp        # SDL window + swap buffers
│   └── renderer/            # OpenGL rendering pipeline
│       ├── camera.h/.cpp    # Grid camera with lerp animation
│       ├── debug_renderer.h/.cpp # Line/shape debug overlay
│       ├── framebuffer.h/.cpp    # FBO with depth attachment
│       ├── frustum.h/.cpp   # View-frustum culling
│       ├── material.h/.cpp  # Shader + texture binding
│       ├── mesh.h/.cpp      # Vertex/index buffers, OBJ/GLTF loader
│       ├── renderer.h/.cpp  # Instanced batched renderer
│       ├── shader.h/.cpp    # GLSL compile/link/uniform cache
│       └── texture.h/.cpp   # 2D textures, STB image loader
├── game/                    # Game logic
│   ├── game_app.h/.cpp      # Main loop, FBO pipeline, ImGui init
│   ├── game_mode.h          # GameMode enum
│   ├── grid_position.h      # GridPosition struct
│   ├── direction.h          # Direction enum + helpers
│   ├── combat/              # Combat & RPG systems
│   │   ├── action.h         # ActionType enum
│   │   ├── character.h      # Hero stats
│   │   ├── combat_system.h/.cpp # Attack resolution, dice rolls
│   │   ├── dice.h/.cpp      # d20/d6 random rolls
│   │   ├── monster.h        # Monster stats + AI type
│   │   ├── monster_manager.h/.cpp # Spawn/despawn/lookup
│   │   ├── monster_renderer.h/.cpp # Billboard sprite renderer
│   │   └── turn_queue.h     # Turn queue
│   ├── dungeon/             # Dungeon & level systems
│   │   ├── chunk.h          # Cell + Chunk (35×35 grid)
│   │   ├── dungeon.h/.cpp   # Dungeon grid, walkability
│   │   ├── dungeon_renderer.h/.cpp # Geometry generation + rendering
│   │   └── tile.h           # TileType enum (unused, Cell used instead)
│   ├── states/              # Game states
│   │   ├── main_menu_state.h/.cpp
│   │   ├── play_state.h/.cpp      # Main gameplay
│   │   └── settings_state.h/.cpp  # Settings + JSON config
│   └── ui/                  # UI layer
│       └── combat_log.h/.cpp      # Coloured combat log
├── 3rdparty/                # Third-party libraries
│   ├── cgltf/               # GLTF mesh loader
│   ├── glad/                # OpenGL loader
│   ├── glm/                 # OpenGL Mathematics
│   ├── imgui/               # Dear ImGui (debug UI)
│   ├── nlohmann/            # JSON library
│   ├── SDL3/                # SDL3 runtime + headers
│   ├── stb/                 # stb_image, stb_vorbis
│   └── tiny_obj_loader/     # OBJ mesh loader
├── stdafx.h                 # Precompiled header
├── stdafx.cpp               # PCH creator
├── main.cpp                 # Entry point
├── Game.slnx                # Visual Studio solution
└── Game.vcxproj             # Visual Studio project
```

---

## Build

### Prerequisites
- **g++ (MinGW)**: C++26 support
- **SDL3**: included in `src/3rdparty/SDL3/`
- **OpenGL 3.3**: compatible GPU

### Windows (MinGW)
```
build.bat
```

### Windows (MSVC)
Open `src/Game.slnx` in Visual Studio 2022+ and build.

### Output
`bin/dungeon_crawlers.exe`

---

## Dependencies

All in `src/3rdparty/`:
- SDL3, glad, GLM, Dear ImGui, nlohmann/json, stb, cgltf, tinyobjloader

---

## Code Style

See [AGENTS.md](./AGENTS.md).

---

## Documentation

- [FINISH.md](./FINISH.md) — completed steps (1–44)
- [ROADMAP.md](./ROADMAP.md) — future tasks
- [AUDIT.md](./AUDIT.md) — code audit and known issues
- [AGENTS.md](./AGENTS.md) — coding conventions

---

## License

MIT
