# DOOM OpenGL — Computer Graphics Team Project

A **work-in-progress** retro first-person graphics project in C++17 and OpenGL 3.3.
The current branch is a mechanics *foundation*, not a complete game or final levels.

## Requirements

- CMake 3.20+ and a C++17 compiler (MSVC, Clang, or GCC)
- OpenGL 3.3 Core support
- Git and network access for the first CMake configuration:
  GLFW 3.4, GLAD 0.1.36, and nlohmann/json 3.11.3 are fetched automatically

## Build

Windows (Visual Studio 2022 / PowerShell):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\Debug\doom_opengl.exe
```

macOS / Linux (from the repository root):

```bash
cmake -S . -B build
cmake --build build
./build/doom_opengl
```

You can also start the game from the build directory because CMake copies the test arena there.

## Controls

| Input | Action |
| --- | --- |
| WASD | Move and slide against walls |
| Mouse | Look around |
| Space | Jump |
| Left click | Shoot (ray hit test blocked by walls and closed doors) |
| E | Toggle nearby door |
| F | Toggle flashlight |
| R | Restart test level |
| Esc | Exit |

The simple enemies move directly toward you when close enough. This is intentionally **not** full AI.

## Level maker & test arena

Open [`tools/level-maker.html`](tools/level-maker.html) in any browser. It works offline.
Choose a tile type, click/drag to paint the grid, then **Export JSON**.
Copy the JSON into `assets/levels/` and launch the game with its file path:

```bash
./build/doom_opengl assets/levels/my_level.json
```

The included [`assets/levels/test_arena.json`](assets/levels/test_arena.json) has two rooms
connected by a door, two enemies, a health pickup, and an exit for testing mechanics.

Level JSON format:

```json
{
  "name": "Tiny Example",
  "tiles": [
    "######",
    "#P...#",
    "#.D..#",
    "#H.MX#",
    "######"
  ]
}
```

Symbols:

- `#` — solid wall
- `.` — empty floor
- `P` — player spawn (exactly one)
- `D` — closed, interactable door
- `M` — simple enemy spawn
- `H` — health pickup
- `X` — exit (exactly one)

All rows must have the same length. Tiles are 2×2 world units.
Validate the layouts using:

```bash
python3 tools/validate_levels.py
```

## Graphics already present

Perspective FPS camera, OpenGL VAO/VBO meshes, shader-based procedural brick/tile
materials, basic diffuse and ambient lighting, flashlight cone, 2D HUD,
and 320×180 retro pixel framebuffer.

## Team TODO (deliberately unfinished)

- Design your own levels, artwork, textures, and models
- Enemy AI, pathfinding, animation, and additional enemies
- More sophisticated weapons, sound, and objectives
- Graphics/debug views, visual polishing, performance optimization
- Final report, demo, and exam presentation

## Code layout

- `src/main.cpp`: current gameplay loop and rendering foundation
- `src/Level.hpp`, `src/Level.cpp`: validated JSON level loading, tiles, collision, doors, pickups
- `tools/level-maker.html`: simple browser-based grid editor (no game dependency)
- `tools/validate_levels.py`: layout checks and exit reachability

This is an intermediate refactor: graphics, input, and gameplay in `main.cpp`
should be split into modules as the team adds features.
