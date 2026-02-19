# Physically Based Animations

Resume/portfolio project focused on real-time rigid-body simulation, interactive tooling, and engine architecture in modern C++.

## What It Does
- Simulates oriented rigid boxes with linear and angular motion, collisions, friction, restitution, damping, and sleeping.
- Ships with 12 built-in demo/stress scenes (including dense-contact cases like `Cube Cloud (1200)`).
- Provides interactive editor workflows:
  - click and shift-click selection,
  - drag-box selection (including optional through-depth selection with `Ctrl/Cmd`),
  - multi-object grab/move with axis constraints (`G`, then `X/Y/Z`).
- Includes a dedicated physics debug panel:
  - broadphase and narrowphase counters,
  - contact marker/normal overlays,
  - selected-body velocity and angular-velocity vectors,
  - kinetic-energy tracking and history plotting.
- Records viewport output directly to MP4 (`ffmpeg`) from the app.
- Supports headless simulation runs for repeatable scene/step benchmarking.

![Interactivity](screenshots/Interactivity.png)
![Physics Debug](screenshots/PhysicsDebug1.png)
![Physics Debug](screenshots/PhysicsDebug2.png)
![ThemeSelection](screenshots/ThemeSelection.png)

## Technical Summary
### Physics
- Fixed-step simulation (`45 Hz`) with accumulator-based stepping.
- Broadphase: sweep-and-prune style candidate generation on AABBs.
- Narrowphase: OBB-vs-OBB SAT testing across face and edge cross axes.
- Contact handling:
  - multi-point contact generation (reduced to up to 4 points),
  - iterative sequential impulse solve,
  - Coulomb friction + restitution,
  - warm starting with frame-to-frame contact cache,
  - positional correction iterations.
- Data layout is SoA for body state; parallel loops can use OpenMP when enabled.

### Rendering and Tools
- OpenGL 4.1 core rendering path with offscreen viewport FBO.
- ImGui-based docked editor (scene browser, inspector, render settings, terminal, physics debug).
- JSON-driven theme loading (`assets/ui/themes.json`) with live theme switching.
- Built-in video capture pipeline (`RGBA` framebuffer readback -> `ffmpeg` -> H.264 MP4).

## Controls
- `LMB`: select object
- `Shift + LMB`: add/remove from selection
- `LMB drag`: box select
- `Ctrl/Cmd + LMB drag`: through-depth box select
- `G`: begin grab/move
- `X` / `Y` / `Z`: constrain grab axis
- `Enter` or `LMB`: confirm grab
- `Esc` or `RMB`: cancel grab
- `MMB drag` or `R + drag`: orbit camera
- `Shift + MMB drag`: pan camera pivot
- Mouse wheel: zoom
- `Space`: pause/resume simulation
- `F2`: start/stop recording
- `F3`: reveal physics debug window

## Build and Run
### Requirements
- CMake `>= 3.20`
- C++23 compiler
- `ffmpeg` available in `PATH` (required by CMake configure)

### Configure + Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Run GUI
```bash
./build/main
```

### Run Headless
```bash
./build/headless --help
./build/headless --scene 2 --steps 5000 --progress
```

You can also run headless mode through the GUI binary:
```bash
./build/main --headless --scene 2 --steps 5000 --progress
```

## Scope Notes
- Current simulation primitive is rigid cubes/boxes.
- Scene save/load and undo/redo menu entries are placeholders.

## Videos
- https://www.youtube.com/watch?v=1fZnTQ-wU24
- https://www.youtube.com/watch?v=uKSDHguqzIE
- https://www.youtube.com/watch?v=C_mCMDn9qz0

## References
- [PBRT4] Physically Based Rendering, 4th Edition: https://pbr-book.org/4ed/
- [Catto05] Iterative Dynamics with Temporal Coherence: https://box2d.org/files/ErinCatto_IterativeDynamics_GDC2005.pdf
- [Baraff97] Physically Based Modeling (SIGGRAPH Notes): https://www.cs.cmu.edu/~baraff/sigcourse/
- [Ericson04] Real-Time Collision Detection: https://realtimecollisiondetection.net/
- [Box2D] Box2D Physics Engine: https://github.com/erincatto/box2d
- [Fiedler04] Fix Your Timestep!: https://gafferongames.com/post/fix_your_timestep/
- [OpenGLRef] Khronos OpenGL Reference Pages: https://registry.khronos.org/OpenGL-Refpages/
- [GSL] Microsoft Guidelines Support Library: https://github.com/microsoft/GSL

## Asset Credits
- PolyHaven:
  - https://polyhaven.com/a/marble_bust_01
  - https://polyhaven.com/a/clean_asphalt
  - Environment map (`assets/textures/environment/studio_env_latlong.png`):
    - Studio Small 09 HDRI by Sergej Majboroda: https://polyhaven.com/a/studio_small_09
    - License: CC0 1.0 (Poly Haven asset license): https://polyhaven.com/license
    - Source download used: https://dl.polyhaven.org/file/ph-assets/HDRIs/extra/Tonemapped%20JPG/studio_small_09.jpg
- Monaspace:
  - https://monaspace.githubnext.com
