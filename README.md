# DOOM OpenGL — Computer Graphics Team Project

A **work-in-progress** retro first-person graphics project in C++17 and OpenGL 3.3.
This is a mechanics *foundation*, not a complete game or final levels.

## Requirements

- CMake 3.20+ and a C/C++17 toolchain (MSVC, Clang, or GCC)
- OpenGL 3.3 Core support
- Git and network access for the first CMake configuration:
  GLFW 3.4 and nlohmann/json 3.11.3 are fetched automatically.
- GLAD is compiled from GLFW's pre-generated OpenGL 3.3 bindings. Python is not required to configure, build, or run.

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

You can also start from the build directory because CMake copies all included maps there.

## Controls

| Input | Action |
| --- | --- |
| WASD | Move and slide against walls |
| Mouse | Look around |
| Space | Jump / double jump |
| Left click | Shoot (ray hit test blocked by walls and closed doors) |
| E | Toggle nearby door |
| F | Toggle flashlight |
| R | Reload |
| T | Restart level |
| Ctrl | Slide |
| 1 / 2 | Pistol / shotgun (after pickup) |
| Q / mouse wheel | Cycle unlocked weapons |
| M | Mute / unmute sound effects |
| Esc | Pause / settings / resume |
| F11 | Toggle fullscreen |
| Arrow keys / Enter / mouse | Navigate pause menu |

The simple enemies move directly toward you when close enough. This is intentionally **not** full AI.

## Settings

Press Esc to pause the game. The settings menu can be operated with the
mouse or arrow keys and Enter. F11 toggles desktop fullscreen. Window size
presets: 960×540, 1280×720, 1600×900, 1920×1080, 2560×1440. On macOS, F11
and the in-game display mode use native AppKit fullscreen, matching the green
title-bar button (and Control-Command-F). The menu detects OS fullscreen
changes automatically and waits for the transition to finish. Windows/Linux
use GLFW monitor fullscreen. Windowed dimensions are read from the OS;
fullscreen exit restores the windowed geometry. The internal render resolution is fixed at 180p (retro), independent of window size.
The renderer preserves the window aspect ratio, including HiDPI displays.
VSync stays enabled; sound can be toggled, and the current level can be restarted or
the game closed. Settings apply immediately and last for the current session.

## Four-level campaign

Launch the game without arguments to play the campaign in order:

1. **Outer Bunker** — two doors, flat rooms, demons, shotgun, and a health pickup.
2. **Molten Foundry** — lava channels with safe walkable bridges and a flying enemy.
3. **Inferno Ascent** — broad floating platforms over lava, gentle height progression, and a raised exit.
4. **The Titan** — dedicated boss arena with cover pillars, a shotgun and medkits. The 850 HP armored boss resists pistol damage, fires faster projectiles, and becomes faster with a three-shot spread below half health. A pistol shot uses one ray (40 damage to regular enemies, 25 to the Titan); the shotgun uses five pellets with distance falloff.

Eliminate every enemy to unlock the exit (red becomes green); touch the unlocked exit to progress. The fourth exit completes the campaign.
Health and inventory reset at each stage, with new pickups provided.
Use T to restart the current stage.

To start at a specific stage, pass `assets/levels/level2.json` or
`assets/levels/level3.json` or `assets/levels/level4.json` to the executable. Other map filenames are standalone
test levels. The original test arena and collision sample remain available.

## Level maker & test arena

Open [`tools/level-maker.html`](tools/level-maker.html) in any browser. It works offline.
Choose a tile or platform brush, click/drag to paint the grid, inspect the orbitable 3D preview, then **Export JSON**. Use the platform brush to set a top height and bottom height independently of the tile/entity. Undo/redo and all four campaign maps plus the built-in platforming sample are available.
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
- `S` — shotgun pickup; `F` — flying enemy; `B` — heavy enemy
- `~` — lava pit (no floor; falling into it kills the player)

All rows must have the same length. Tiles are 2×2 world units.
The game validates levels in C++ when loading them (including exit reachability).
You can also validate JSON files **without launching the game** using the
standalone C++ executable included in the build:

Windows:
```powershell
.\\build\\Debug\\level_validator.exe assets\\levels\\test_arena.json
```

macOS / Linux:
```bash
./build/level_validator assets/levels/test_arena.json
```

Add more file paths to validate multiple levels. It exits with a nonzero status
for invalid files, making it useful in automated build checks.
No Python installation is required.

## Platforming

Try the included Inferno Escape sample:

```bash
./build/doom_opengl assets/levels/platforming.json
```

Windows: `.\build\Release\doom_opengl.exe assets/levels/platforming.json`.

The optional `ceilingHeight` is in world units (4.5–32, default 4.5).
The optional `platforms` array contains objects like
`{"column":4,"row":3,"bottom":0.4,"top":0.7}`.
Coordinates are zero-based, each footprint is one 2×2 tile. Set bottom to 0
for a solid raised block, or above 0 for a floating platform. Paint lava
under gaps. One platform per tile; a spawn, pickup, enemy or exit on a
platform uses its top as the entity's base height. Older maps still load.
The editor's reachability hint is approximate; the native validator checks
shape and non-wall connectivity, not whether each possible jump is feasible.
Playtest exported levels. Custom maps with names other than level1.json, level2.json, level3.json and level4.json finish at their own exit.
Run `ctest --test-dir build -C Release --output-on-failure` for collision tests.

## Audio

Original retro sound effects are synthesized at startup and played by miniaudio 0.11.21 (fetched at configuration). No external audio files or Python are required. If audio initialization fails, gameplay continues silently. M toggles sound. Each enemy type has its own growl or hiss, pain and death sound; heavy enemies have footsteps, melee enemies have attack cues, and flyers use a projectile cue. Enemy sounds use distance attenuation and stereo panning at trigger time. Both weapon slots stay visible in the HUD, including the locked shotgun, with keybinds and ammo.

## Graphics already present

Perspective FPS camera, OpenGL VAO/VBO meshes, shader-based procedural brick/tile
materials, basic diffuse and ambient lighting, flashlight cone, 2D HUD,
and a fixed 180p retro pixel framebuffer with a native-resolution HUD.

## Procedural retro models

The model update uses the existing cube VAO with independently transformed parts
and solid-color materials. No Blender assets or new runtime libraries are required.
The three enemy types (M normal, F flying, B heavy), medkit (H), shotgun pickup (S),
doors and projectiles have new models. The pistol and shotgun use a separate 3D
camera-space weapon pass, with recoil, reload and movement bob. Enemy leg movement
and wing flapping are visual only; hitboxes and gameplay remain unchanged.

Test all four campaign levels: look at each enemy, shoot it, pick up the medkit/shotgun, switch
weapons, reload, toggle doors, and walk/slide/jump. Verify that all four campaign levels are
present in the assets/levels directory before starting the game.

## Team TODO (deliberately unfinished)

- Design additional levels, artwork and textures
- More advanced enemy AI, pathfinding, and additional enemies
- More sophisticated weapons, music, and objectives
- Graphics/debug views, visual polishing, performance optimization
- Final report, demo, and exam presentation

## Code layout

- `src/main.cpp`: current gameplay loop and rendering foundation
- `src/Models.hpp`: procedural model geometry, materials and visual animation
- `src/Level.hpp`, `src/Level.cpp`: validated JSON level loading, tiles, collision, doors, pickups
- `tools/level-maker.html`: simple browser-based grid editor (no game dependency)
- `tools/validate_levels.cpp`: native C++ command-line validator, sharing `Level::load()` with the game

This is an intermediate refactor: graphics, input, and gameplay in `main.cpp`
should be split into modules as the team adds features.
