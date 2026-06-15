# Dungeon Crawlers

A retro-style dungeon crawler RPG in the vein of *Eye of the Beholder*, built from scratch in C++26 with OpenGL 3.3 and SDL3.

> **Current status**: 3D engine skeleton with grid-based movement, procedural dungeon rendering, turn-based roguelike combat prototype.

---

## Features

### Implemented
- **3D renderer**: OpenGL 3.3 core profile, instancing, frustum culling, UBO for view/projection
- **Grid-based movement**: snap-to-grid camera with lerp animation (0.2s per step), 90° turns
- **Dungeon**: procedural tile grid with walls, floor, ceiling rendered as instanced meshes
- **Turn-based exploration**: player moves one cell per turn, input blocked during animation
- **Combat**: melee attack (E key), d20-based hit formula, damage range, critical hits
- **Monsters**: billboard-sprites, stationary/patrol/aggressive AI, death handling
- **Party system**: up to 6 characters, formations (line/double-line/single)
- **Character stats**: HP, AC, STR/DEX/CON/INT/WIS/CHA, equipment, inventory
- **Items**: weapon/armor/shield/potion/scroll/key, pick up (G), use, drop
- **Spells**: Magic Missile, Heal, Light, Shield — mana-based, from spellbook or scroll
- **Levels**: multi-floor dungeon with stairs, locked doors, secret walls, traps
- **FOV**: raycasting-based field-of-view, fog of war (unseen / explored / visible)
- **Retro FBO**: low-resolution framebuffer (320×240) with nearest-neighbour upscale
- **Combat log**: colored message log (hit, miss, critical, death, spell)
- **HUD**: HP bars, AC, minimap, compass, party portraits
- **Save/Load**: JSON serialization via nlohmann (F5 save, F9 load)
- **Settings**: mouse sensitivity, sound volume, render resolution, fullscreen toggle
- **Audio**: OGG/WAV playback via SDL3 audio streams, ambient + SFX channels
- **Debug tools**: Profiler (timings), wireframe overlay, camera debug, ImGui panels
- **Cross-platform**: `#ifdef __EMSCRIPTEN__` guards, no `filesystem`, no `thread` on Web

### Planned
See [ROADMAP.MD](./ROADMAP.MD) for the full 120-step development plan.

---

## Build

### Prerequisites
- **g++ (MinGW)**: C++26 support
- **SDL3**: included in `src/3rdparty/SDL3/`
- **OpenGL 3.3**: compatible GPU and drivers

### Windows (MinGW)
```
build.bat
```
The script compiles the precompiled header, all source files, links against SDL3 and OpenGL, and copies `SDL3.dll` to `bin/`.

### Windows (MSVC)
Open `src/Game.slnx` in Visual Studio 2022+ and build.

### Web (Emscripten) — not yet set up
See ROADMAP for WASM build target.

### Output
The executable is placed in `bin/dungeon_crawlers.exe`.

---

## Controls

| Key | Action |
|-----|--------|
| W / S | Move forward / backward (grid step) |
| A / D | Turn left / right (90°) |
| E | Melee attack (adjacent enemy) |
| G | Pick up item from floor |
| I | Open inventory |
| F | Search (secret doors, traps) |
| R | Rest (heal HP/MP) |
| Tab | Toggle minimap / debug overlay |
| F5 | Quick save |
| F9 | Quick load |
| C | Character stats screen |
| Space | End turn / continue dialog |
| Escape | Release mouse / quit |

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
│   ├── audio_system.h/.cpp  # WAV/OGG playback, music + SFX streams
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
│       ├── frustum.h/.cpp   # View-frustum culling (6 planes)
│       ├── material.h/.cpp  # Shader + texture binding
│       ├── mesh.h/.cpp      # Vertex/index buffers, OBJ/GLTF loader
│       ├── renderer.h/.cpp  # Instanced batched renderer
│       ├── shader.h/.cpp    # GLSL compile/link/uniform cache
│       └── texture.h/.cpp   # 2D textures, checkerboard, STB image
├── game/                    # Game logic
│   ├── game_app.h/.cpp      # Main loop, FBO pipeline, ImGui init
│   ├── combat/              # Combat & RPG systems
│   │   ├── action.h         # Action types enum
│   │   ├── character.h      # Hero stats, inventory, equipment
│   │   ├── combat_system.h/.cpp # Attack resolution, dice rolls
│   │   ├── dice.h           # d20/d6 random rolls
│   │   ├── inventory.h/.cpp # Item container
│   │   ├── item.h           # Item types, modifiers
│   │   ├── monster.h        # Monster stats + AI type
│   │   ├── monster_manager.h/.cpp # Spawn/despawn/update
│   │   ├── monster_renderer.h/.cpp # Billboard sprite renderer
│   │   ├── party.h/.cpp     # Party management, formations
│   │   ├── spell.h          # Spell definitions
│   │   └── turn_queue.h/.cpp # Initiative-ordered turn queue
│   ├── dungeon/             # Dungeon & level systems
│   │   ├── dungeon.h/.cpp   # Tile grid, level data, walkability
│   │   ├── dungeon_renderer.h/.cpp # Wall/floor/ceiling geometry
│   │   ├── tile.h           # TileType, Tile struct
│   │   ├── trap.h           # Trap definitions
│   │   └── trigger.h        # Event triggers
│   ├── states/              # Game states (screens)
│   │   ├── main_menu_state.h/.cpp # Main menu (Start/Settings/Exit)
│   │   ├── play_state.h/.cpp      # Main gameplay state
│   │   └── settings_state.h/.cpp  # Settings with JSON config
│   └── ui/                  # Retro UI layer
│       ├── bitmap_font.h/.cpp     # Bitmap font rendering
│       ├── combat_log.h/.cpp      # Colored combat log
│       └── hud.h/.cpp             # HP bars, minimap, compass
├── 3rdparty/                # Third-party libraries
│   ├── cgltf/               # GLTF mesh loader
│   ├── glad/                # OpenGL loader
│   ├── glm/                 # OpenGL Mathematics
│   ├── imgui/               # Dear ImGui (debug UI)
│   ├── nlohmann/            # JSON library
│   ├── SDL3/                # SDL3 runtime + headers
│   ├── stb/                 # stb_image, stb_vorbis, stb_truetype
│   └── tiny_obj_loader/     # OBJ mesh loader
├── stdafx.h                 # Precompiled header + all includes
├── stdafx.cpp               # PCH creator
├── main.cpp                 # Entry point
├── Game.slnx                # Visual Studio solution
└── Game.vcxproj             # Visual Studio project
```

---

## Dependencies

All third-party libraries are included in the repository under `src/3rdparty/`:

| Library | Version | Purpose |
|---------|---------|---------|
| [SDL3](https://github.com/libsdl-org/SDL) | 3.2.x | Window, input, audio |
| [glad](https://github.com/Dav1dde/glad) | — | OpenGL 3.3 loader |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Math (vectors, matrices, quaternions) |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.92 | Debug UI panels |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11 | JSON serialization |
| [stb](https://github.com/nothings/stb) | latest | Image loading, OGG decoding, font baking |
| [cgltf](https://github.com/jkuhlmann/cgltf) | latest | GLTF mesh loading |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | latest | OBJ mesh loading |

---

## Code Style

See [AGENTS.md](./AGENTS.md) for full coding conventions. Key rules:
- C++26, `final` classes, `[[nodiscard]]`, Concepts, `std::span`/`std::string_view`
- Allman braces, tabs for indentation
- `PascalCase` for types/methods, `camelCase` for private members with `m_` prefix
- `#pragma once`, no trailing return types, no `typedef`, no ECS
- Cross-platform: `#ifdef __EMSCRIPTEN__` guards, no `filesystem`, no `thread` on Web

---

## Documentation

- [ROADMAP.MD](./ROADMAP.MD) — 120-step development plan
- [AUDIT.MD](./AUDIT.MD) — current issues, optimization, and code audit
- [AGENTS.MD](./AGENTS.MD) — coding conventions and toolchain rules

---

## License

MIT License — see repository root for details.
