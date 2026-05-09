# Godot Engine Migration Guide

## Overview

This document outlines a hybrid migration strategy for Chicken Potato FPS from Raylib to Godot Engine. The recommended approach retains the existing C++ gameplay core (game logic, networking, AI, physics) and uses Godot primarily for rendering, UI, input handling, and scene management via **GDExtension** (Godot's C++ extension system). This hybrid approach maximizes code reuse, preserves network protocol compatibility, and leverages Godot's mature renderer and editor tooling without requiring a complete gameplay rewrite.

## Goals and Non-Goals

### Goals
- Migrate rendering and windowing from Raylib to Godot (DirectX 12, Metal, Vulkan support).
- Migrate UI and menu systems to Godot scenes and Control nodes.
- Migrate input handling (keyboard, mouse, touch) to Godot's InputMap and InputEvent system.
- Retain existing C++ gameplay systems (wave logic, chicken AI, projectile physics, damage, networking).
- Maintain deterministic gameplay behavior and network snapshot compatibility with existing online clients.
- Support Android, Windows, Linux, macOS, and Web (via Godot exports).
- Preserve or improve performance and reduce platform-specific code complexity.

### Non-Goals (Phase 1)
- Complete rewrite of gameplay logic in GDScript or C#.
- Migration to Godot's built-in physics (PhysicsServer3D) — manual collision checks remain for compatibility.
- Immediate port of Emscripten/Web build to Godot (can be done in Phase E, post-stability).
- Replacement of custom UDP networking with ENetMultiplayerPeer (keep custom protocol initially, evaluate ENet post-Phase D).

## Current Architecture (Raylib)

### Monolithic Game Loop
The current implementation ([src/main.cpp](src/main.cpp#L1523)) contains a single `while (!shouldExit)` loop that:
- Handles window events and input (keyboard, mouse, touch).
- Updates game state (wave spawning, entity movement, collision, damage).
- Sends/receives network packets (host-authoritative snapshots).
- Draws 3D scene, UI overlays, and touch controls.

### Key Subsystems
| Subsystem | Current Location | Responsibility |
|-----------|------------------|-----------------|
| **Game State** | [main.cpp L460–650](src/main.cpp#L460) | Player health, score, wave, inventory, mode flags |
| **Wave/Spawn Logic** | [main.cpp L717–725](src/main.cpp#L717) | Wave difficulty scaling, chicken spawning |
| **Reset/Init** | [main.cpp L727–787](src/main.cpp#L727) | Game reset, inventory setup, highscore init |
| **Network Host** | [main.cpp L908–935](src/main.cpp#L908) | UDP socket, listen, client connection |
| **Network Client** | [main.cpp L936–969](src/main.cpp#L936) | UDP socket, join via IP, send input |
| **Network Packets** | [main.cpp L970–1132](src/main.cpp#L970) | Snapshot encoding/decoding, packet routing |
| **Highscores** | [main.cpp L328–407](src/main.cpp#L328) | File I/O, persistence, submission |
| **Touch Controls** | [main.cpp L1288–1480](src/main.cpp#L1288) | Joystick, button states, tap/hold detection |
| **Menu/Intro UI** | [main.cpp L1643–1748](src/main.cpp#L1643) | Main menu, mode selection, highscores view |
| **Online Setup UI** | [main.cpp L1752–1877](src/main.cpp#L1752) | Host/join dialog, IP input |
| **Pause Menu** | [main.cpp L1903–1960](src/main.cpp#L1903) | Resume, resolution, fullscreen toggles |
| **Shop/Inventory UI** | [main.cpp L3091–3315](src/main.cpp#L3091) | Weapon/upgrade purchases, slot management |
| **Gameplay Update** | [main.cpp L2138–2862](src/main.cpp#L2138) | Movement, jump, sprint, weapon cooldowns, entity updates |
| **Rendering 3D** | [main.cpp L2899–3050](src/main.cpp#L2899) | BeginMode3D, chicken/projectile/weapon meshes, shadows |
| **HUD Overlay** | [main.cpp L3055–3089](src/main.cpp#L3055) | Health, score, wave, chickens left, weapon indicator |

---

## Target Hybrid Architecture

### Responsibility Split

#### **C++ Gameplay Core (Retained)**
- **Game State & Logic**: Player health, coopHealth, position, inventory, wave/score tracking.
- **Entity Systems**: Chicken spawning, movement, AI targeting, damage application.
- **Physics & Collision**: Manual distance checks for projectile/enemy hits (no physics engine integration).
- **Networking**: UDP packet construction, snapshot encoding/decoding, host-authoritative game loop.
- **Highscores**: File persistence and entry management.
- **Audio Events**: Firing, hit, death audio trigger signals sent to Godot.

#### **Godot Presentation Layer (New)**
- **Rendering**: 3D meshes, materials, lighting, shadows, post-processing (damage flash, shield vignette).
- **Camera Control**: CharacterBody3D + Camera3D node hierarchy, mouse/touch look input routing.
- **Menu/UI Scenes**: Main menu, online setup, pause, shop, inventory, death screen, highscore entry.
- **Input Routing**: InputMap, InputEvent handling, joystick detection, virtual button feedback.
- **Touch Controls**: Virtual joystick and buttons as CanvasLayer controls (visual representation).
- **Audio Playback**: AudioStreamPlayer3D/2D for SFX and music (triggered by C++ signals).
- **Scene Management**: Level transitions, screen state (Intro → OnlineSetup → Playing → Death).

### Data Flow Model

```
┌─────────────────────┐
│  Godot Input Layer  │
│  (InputMap, events) │
└──────────┬──────────┘
           │ keyboard, mouse delta, touch positions
           ▼
┌──────────────────────────────────────────┐
│  C++ Gameplay Core (GDExtension Module)  │
│  ┌───────────────────────────────────┐  │
│  │ update(delta, input_state) {      │  │
│  │   - move player                   │  │
│  │   - update entities               │  │
│  │   - apply damage                  │  │
│  │   - send/recv network packets     │  │
│  │ }                                 │  │
│  │                                   │  │
│  │ get_render_state() -> {           │  │
│  │   positions, health, enemies, ... │  │
│  │ }                                 │  │
│  └───────────────────────────────────┘  │
└──────────────────────────────────────────┘
           │ render state, audio triggers, UI updates
           ▼
┌──────────────────────────┐
│  Godot Render / UI Layer │
│  (Scenes, nodes, canvas) │
└──────────────────────────┘
```

---

## System Mapping: Raylib → Godot + C++

### Camera & View
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| `Camera3D camera` (Raylib struct) | `Camera3D` node in Godot scene | C++ passes yaw/pitch angles to Godot; Godot node rotates. |
| `GetMouseDelta()` | `InputEventMouseMotion.relative` | Godot InputEvent routed to C++ input handler. |
| `GetTouchPosition(i)`, `GetTouchPointCount()` | `InputEventScreenTouch`, `InputEventScreenDrag` | Godot InputEvent with screen position and ID; C++ processes joystick/button hit-tests. |
| `BeginMode3D(camera)` / `EndMode3D()` | Godot 3D viewport (automatic in scene tree) | 3D rendering handled by Godot engine; no explicit Begin/End calls. |

### Rendering
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| `DrawCube`, `DrawSphere`, etc. | `MeshInstance3D` + `StandardMaterial3D` | Pre-built 3D models or procedurally generated meshes stored in `.tres` files. |
| `DrawText` (2D overlay) | `Label`, `RichTextLabel` nodes | Godot UI nodes; C++ provides text content via `set_text()`. |
| `BeginDrawing()` / `EndDrawing()` | Automatic frame rendering | Godot engine handles frame buffering and swap. |
| `ClearBackground()` | `WorldEnvironment` node (sky/background color) | Set in scene or via C++ code during level setup. |
| `DrawRectangle`, `DrawRectangleRounded` | `ColorRect`, `Panel` nodes | For UI backgrounds and debug visuals. |

### Input
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| `IsKeyPressed(KEY_W)` | `Input.is_action_pressed("move_forward")` | Define actions in InputMap; C++ queries state in update loop. |
| `GetMousePosition()` | `get_global_mouse_position()` | Called on viewport or input listener node. |
| `IsMouseButtonPressed(MOUSE_BUTTON_LEFT)` | `Input.is_action_just_pressed("shoot")` or `InputEventMouseButton` | Map mouse buttons to actions in InputMap. |
| `GetCharPressed()`, `GetKeyPressed()` | `InputEventKey.keycode` or text input focus on LineEdit | For highscore initials, use LineEdit node or custom text input. |

### UI & Menus
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| Manual menu rect drawing + hover checks | `Button`, `Panel`, `VBoxContainer` scenes | Godot UI framework; C++ provides state and callbacks. |
| Pause menu with resolution options | `PauseMenu` scene (Control node hierarchy) | Resolution changes via `get_window().set_size()`. |
| Shop and inventory layouts | `Shop` and `Inventory` scenes | Godot Control nodes for layout; C++ manages item state. |
| Highscore entry with `GetCharPressed()` | `LineEdit` node with text_submitted signal | Connect signal to C++ callback for score submission. |
| Touch button drawing (immediate mode) | `TouchUI` CanvasLayer with Control nodes | Virtual joystick and buttons as persistent UI nodes. |

### File I/O & Persistence
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| `std::ifstream` / `std::ofstream` (root dir) | `FileAccess` + `user://` virtual path | Godot provides sandboxed user data directory; C++ uses FileAccess from GDExtension. |
| `highscores.txt` in project root | `user://highscores.txt` (platform-specific user dir) | Ensures Android/iOS/Web permission compliance. |

### Networking
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| Raw socket + `sendto` / `recvfrom` | Keep C++ UDP code; expose state to Godot | Initial phase: no change to packet format or protocol logic. C++ sockets remain; Godot displays status. |
| Network status display | `Label` node or HUD overlay | C++ updates status string; Godot renders it. |
| Client/host switching | Game state flags in C++ | Godot UI triggers host/join actions; C++ manages socket lifecycle. |

### Platform-Specific Code
| Raylib | Godot + C++ | Migration Notes |
|--------|-------------|-----------------|
| `#ifdef PLATFORM_ANDROID` (touch layout) | Godot scene-based responsive UI | Godot handles screen size changes; single C++ code path for input. |
| Android NDK integration (`android_app`, JNI) | Godot Android plugin system | For keyboard/IME integration, use Godot's Plugin template. |
| Fullscreen toggle `ToggleFullscreen()` | `get_window().set_mode(Window.MODE_FULLSCREEN)` | Handled by Godot window API. |
| SetTargetFPS(120) | `ProjectSettings.physics_ticks_per_second` | Set in project config; optionally override in code. |

---

## Phased Migration Plan

### Phase A: Extract Engine-Agnostic C++ Core (Parallel with Phase B)
**Goal**: Decouple gameplay logic from Raylib API calls.

**Tasks**
1. Create a new `src/game_core.h` / `src/game_core.cpp` with engine-agnostic data structures and update functions:
   ```cpp
   struct GameState { /* Player, entities, wave data */ };
   struct InputState { /* Keyboard, mouse, touch actions */ };
   struct RenderSnapshot { /* Positions, health, UI state to render */ };
   
   void game_update(GameState& state, const InputState& input, float dt);
   RenderSnapshot game_get_render_state(const GameState& state);
   ```
2. Extract high-level logic from [main.cpp L717–2862](src/main.cpp#L717) (wave, reset, gameplay update, entity updates) into `game_core.cpp`.
3. Isolate Raylib calls to a thin `src/raylib_bridge.cpp` that wraps I/O:
   - File I/O (highscores) → abstracted interface.
   - Network socket creation → abstracted interface.
   - Random number generation → abstracted interface.
4. Create abstract interfaces for platform-specific features:
   ```cpp
   class IPlatformBridge {
       virtual void save_highscores(...) = 0;
       virtual void get_user_data_path(...) = 0;
   };
   ```

**Acceptance Criteria**
- Raylib-free game loop compiles and links.
- Single-player parity preserved in headless test mode.
- Network snapshots remain byte-identical to current implementation.

---

### Phase B: Build Godot Shell & Input Integration (Parallel with Phase A)
**Goal**: Create a playable Godot project with responsive UI and input routing to C++ stubs.

**Tasks**
1. Create a new Godot 4.2+ project at `/godot_project/`:
   ```
   /home/alex/Repos/vibeogame/godot_project/
   ├── project.godot
   ├── scenes/
   │   ├── MainMenu.tscn
   │   ├── OnlineSetup.tscn
   │   ├── GameWorld.tscn
   │   ├── HUD.tscn
   │   ├── Shop.tscn
   │   ├── Inventory.tscn
   │   ├── PauseMenu.tscn
   │   ├── DeathScreen.tscn
   │   └── TouchUI.tscn
   ├── scripts/
   │   ├── GameManager.gd (state machine, C++ integration point)
   │   ├── InputHandler.gd (keyboard/mouse/touch mapper)
   │   └── HUDUpdater.gd (label binding to C++ state)
   ├── assets/
   │   ├── models/ (chicken, player model placeholders)
   │   └── ui/ (button textures, fonts)
   └── gdextension/
       ├── chicken_fps.gdextension (manifest)
       └── ... (C++ binding files, auto-generated)
   ```

2. Set up GDExtension build configuration:
   - Add `scons` build target in CMakeLists.txt to compile C++ module for Godot.
   - Create `CMakeLists.txt` for GDExtension subproject (separate from Raylib build).

3. Create minimal Godot scenes:
   - **MainMenu**: VBoxContainer with buttons for Singleplayer, CoOp, Online, Highscores.
   - **OnlineSetup**: Buttons for Host/Join, LineEdit for IP input.
   - **GameWorld**: A simple 3D scene with a static plane, a CharacterBody3D camera rig, and placeholder cube enemies.
   - **HUD**: Labels for Health, Score, Wave, Chickens Left, Status; a Weapon Indicator.
   - **PauseMenu**: Resume, Resolution, Fullscreen, Quit buttons.
   - **TouchUI** (CanvasLayer): Joystick and buttons drawn as Control nodes (Panels + touch event routing).

4. Connect Godot input signals to C++ callbacks:
   ```gdscript
   # InputHandler.gd
   extends Node
   
   func _ready():
       # Connect input actions to C++ via signal emission
       Input.action_pressed.connect(_on_action)
   
   func _on_action(action: StringName):
       # Call C++ function with action name
       CPPGameCore.handle_input(action)
   ```

5. Create a `GameManager.gd` autoload singleton that manages scene transitions and calls C++ update loop:
   ```gdscript
   extends Node
   
   var cpp_game: Object  # Reference to C++ GameCore instance
   
   func _process(delta):
       var input_state = gather_input_state()
       cpp_game.update(input_state, delta)
       update_ui(cpp_game.get_render_state())
   ```

**Acceptance Criteria**
- Godot project launches with main menu fully interactive.
- UI responds to keyboard/mouse/touch input.
- Scene transitions work (menu → setup → world → pause → death → menu).
- C++ stubs for `update()` and `get_render_state()` compile and run.
- No Raylib dependencies in Godot build.

---

### Phase C: Wire C++ Core via GDExtension & Achieve Parity (Depends on A, B)
**Goal**: Connect extracted gameplay core to Godot; validate single-player and co-op behavior match Raylib version.

**Tasks**
1. Implement GDExtension binding layer (`src/gdextension/game_core_binding.cpp`):
   ```cpp
   class GameCore : public RefCounted {
   public:
       void update(const Dictionary& input, float delta);
       Dictionary get_render_state() const;
       void start_game(int mode);
       void set_resolution(int w, int h);
   };
   ```

2. Bind C++ `GameState` structures to Godot via GDExtension property exposure:
   - Player position, rotation → Camera3D transforms in Godot.
   - Entity positions, health → MeshInstance3D positions and material parameters.
   - UI values (health, score, wave) → Label text updates.

3. Create entity rendering logic in Godot:
   - **Chicken nodes**: Spawn/despawn from C++ entity list; update position/rotation per frame.
   - **Projectile nodes**: Render potatoes; use C++ position data.
   - **Weapon view-model**: CSGMesh or import of gun 3D model; rotate with camera.

4. Implement interaction callbacks:
   - **Fire input** → C++ applies damage, increments weapon cooldown, spawns projectile.
   - **Jump input** → C++ updates vertical velocity.
   - **Wave completion** → C++ signals spawn/difficulty increase → Godot shows banner.

5. Run parity tests:
   - Single-player wave 1-5: Verify chicken spawn count, wave duration, health drops, score gains.
   - Weapon firing: Verify projectile speed, damage, cooldown timing.
   - Player movement: Verify WASD speed matches Raylib, jump height/gravity feel same.
   - UI updates: Verify HUD numbers match gameplay state.

**Acceptance Criteria**
- Single-player gameplay fully playable in Godot; all mechanics function.
- Parity tests pass: wave progression, damage, movement feel identical to Raylib build.
- Co-op mode (two players, local) works with both players responding to inputs.
- Performance baseline: 60+ FPS on target platforms (measure vs. Raylib version).

---

### Phase D: Integrate Online Networking (Depends on C)
**Goal**: Enable host-client online mode while preserving current UDP packet format and snapshot determinism.

**Tasks**
1. Expose network functions in GDExtension:
   ```cpp
   bool setup_host(int port);
   bool setup_client(const String& host, int port);
   void send_input_snapshot(const Dictionary& input);
   ```

2. Wrap raw socket code from [main.cpp L908–1132](src/main.cpp#L908) in C++ network module:
   - Keep `NetHeader`, `NetInputPacket`, `NetSnapshotPacket` structs unchanged.
   - Implement non-blocking receive in `game_update()` to populate `remoteInput` state.
   - Implement host snapshot broadcast logic (same cadence as Raylib: ~30 Hz).

3. Create Godot UI for online mode:
   - **OnlineSetup scene**: Host/Join buttons, IP input LineEdit.
   - **Lobby state**: Show "Waiting for client..." or "Connecting to host..." with timeout logic.
   - **Status label**: Display network latency, packet loss estimate, connection state.

4. Validate snapshot compatibility:
   - Run Godot client against existing Raylib host (or vice versa).
   - Verify packet byte-sequences match current format.
   - Check player interpolation and remote entity state match Raylib experience.

5. Performance tuning:
   - Measure network packet latency and bandwidth usage vs. Raylib version.
   - Ensure UDP socket non-blocking I/O doesn't block Godot frame updates.

**Acceptance Criteria**
- Online multiplayer fully playable (host in Godot, client in Godot, cross-version compatibility).
- Network snapshot format unchanged; byte-for-byte compatibility with current implementation.
- 30+ FPS maintained during online play on LAN and internet (up to 100ms latency).
- Latency display and disconnect handling work correctly.

---

### Phase E: Android & Mobile Polish (Depends on D)
**Goal**: Port touch controls and optimize for mobile devices; ensure feature parity with current Android build.

**Tasks**
1. Implement touch input in `TouchUI.gd`:
   - Virtual joystick (CanvasLayer with Panels): listen to `InputEventScreenTouch` and `InputEventScreenDrag`.
   - Fire button (tap/hold detection from touch_duration and travel distance).
   - Pause, Inventory, Weapon, Scythe, Jump buttons.
   - Reference current tap/hold thresholds from [main.cpp L1375–1425](src/main.cpp#L1375).

2. Port Android-specific initialization:
   - Fullscreen immersive mode via Godot's native plugin interface.
   - DPI scaling and button sizing based on screen metrics.
   - Keyboard/IME integration for highscore entry (use Godot's LineEdit with keyboard filter).

3. Test on Android devices:
   - APK export via Godot Editor (requires Android SDK/NDK setup).
   - Verify touch responsiveness matches current APK.
   - Test landscape/portrait orientation changes.
   - Check battery/thermal behavior under sustained play.

4. Optimize for mobile:
   - Reduce shadow quality/draw calls if frame rate drops below 60 FPS.
   - Profile memory usage; ensure no leaks on scene transitions.
   - Test on low-end hardware (e.g., Snapdragon 765G baseline).

**Acceptance Criteria**
- Android APK builds and runs without crashes.
- Touch controls feel identical to current app in responsiveness and accuracy.
- 60 FPS sustained on mid-range devices (e.g., Pixel 6a, Galaxy S21).
- Highscore entry UI works with on-screen keyboard.
- Portrait and landscape orientations both work (or locked as desired).

---

## Integration Pattern: GDExtension Data Contract

### Input State (Godot → C++)
```cpp
// C++ expects this structure each frame
struct InputState {
    bool move_forward, move_backward, move_left, move_right;
    bool sprint, jump, shoot;
    float mouse_delta_x, mouse_delta_y;
    float touch_camera_delta_x, touch_camera_delta_y;
    bool touch_fire_tap;
    bool touch_fire_hold;
    // ... co-op and online player 2 inputs
};
```

### Render State (C++ → Godot)
```cpp
// C++ returns this each frame for Godot to render
struct RenderSnapshot {
    Vector3 player_pos, player_rotation;
    float player_health;
    
    std::vector<struct {
        Vector3 pos, facing_yaw;
        float health, radius;
        bool is_brown;
    }> chickens;
    
    std::vector<struct {
        Vector3 pos, vel;
        float life, radius;
    }> projectiles;
    
    // UI state
    int score, wave, chickens_left;
    std::string net_status;
    // ... shop, inventory, death state
};
```

### Thread Safety
- **Main thread**: Godot's `_process()` calls C++ `update(input, delta)` and retrieves `RenderSnapshot`.
- **Background (optional)**: Network I/O can run on a separate thread if non-blocking sockets are insufficient; use `Mutex` to protect shared state.
- **Recommendation for Phase 1**: Single-threaded (C++ update in Godot's main thread) for simplicity; migrate to background thread if network latency becomes apparent.

---

## Verification Checklist

### Gameplay Parity
- [ ] Wave 1 spawn count and timing match Raylib version.
- [ ] Weapon fire rate, damage per hit, and projectile speed are identical.
- [ ] Player movement speed (WASD), sprint multiplier, and jump feel (height/gravity) match.
- [ ] Chicken AI targeting and pathing behavior replicates Raylib.
- [ ] Death conditions (health ≤ 0, enemy melee hit) trigger correctly.
- [ ] Score increments (kill, wave complete) are accurate.

### Menu & UI
- [ ] Main menu mode selection works (Singleplayer, CoOp, Online, Highscores).
- [ ] Pause menu resume, resolution change, fullscreen toggle function.
- [ ] Online setup (Host/Join, IP input) succeeds and transitions to game.
- [ ] Shop and Inventory screens display items and allow purchases/swaps.
- [ ] Highscore entry captures 3-letter initials and persists to file.
- [ ] Death screen shows restart and menu buttons.

### Input
- [ ] Keyboard (WASD, mouse, number keys) responds immediately and consistently.
- [ ] Mouse look feels responsive with appropriate sensitivity.
- [ ] Touch on desktop (simulated) and Android device both work.
- [ ] Virtual joystick (Android) provides smooth movement with expected dead-zone behavior.
- [ ] Fire button (mouse left-click, touch hold) triggers weapon correctly.

### Networking
- [ ] Host and client can connect on LAN.
- [ ] Host sends snapshot packets at correct cadence (~30 Hz).
- [ ] Client receives and applies snapshot state without desync.
- [ ] Existing Raylib client can join Godot host and vice versa (cross-version test).
- [ ] Disconnect is detected within 5 seconds and returns to menu.

### Audio
- [ ] SFX triggers for weapon fire, hit, death, UI navigation.
- [ ] Audio latency doesn't noticeably lag behind visuals.
- [ ] No audio playback crashes or glitches.

### Performance
- [ ] 60+ FPS on Windows/Linux (1080p, high-end GPU).
- [ ] 60+ FPS on macOS (1440p, M1/M2 baseline).
- [ ] 60+ FPS on Android mid-range device (Pixel 6a, Galaxy S21).
- [ ] Memory usage stable over 10-minute session (no leaks).
- [ ] No frame stutters during scene transitions or network updates.

### Platform-Specific
- [ ] **Android**: Fullscreen immersive mode, landscape/portrait responsive, touch input accurate.
- [ ] **iOS**: Builds and runs (if Godot export enabled); touch controls work.
- [ ] **Web (Godot Export)**: Game runs in browser at acceptable performance (Phase E+ consideration).
- [ ] **Windows**: Fullscreen/windowed toggle, resolution change smooth.

---

## Rollback & Risk Mitigation

### Fallback Strategy
If Godot integration proves unexpectedly complex or introduces regressions:
1. **Commit Phase A result** (engine-agnostic C++ core) as a separate, stable library.
2. **Keep Raylib build active** as the primary shipping version until Godot parity is 100% verified.
3. **Maintain parallel builds**: CMake targets for both `Raylib` and `GDExtension` versions during Phases B–D.
4. **Branch strategy**: Use feature branch for Godot work; keep `master` on stable Raylib version until Phase D completes.

### Known Risks
| Risk | Mitigation |
|------|-----------|
| GDExtension API changes in future Godot versions | Pin Godot version to 4.2.x; document upgrade path. |
| Network snapshot byte order or precision loss in binding | Extensive cross-version testing; keep raw struct format unchanged. |
| Touch input latency in Godot vs. Raylib | Profile InputEvent processing; consider custom InputProvider if needed. |
| Mobile performance regression (GPU bottleneck) | Pre-profile on target devices; adjust mesh complexity and shadow resolution. |
| Determinism issues in online play | Record and replay game traces; compare snapshots frame-by-frame. |

---

## Build Integration

### Current Raylib Build (Stable)
```bash
cmake -S . -B build && cmake --build build -j$(nproc)
./build/chicken_potato_fps
```

### New GDExtension Build (Godot)
```bash
# Build C++ module
cd /home/alex/Repos/vibeogame
mkdir -p build-gdext
cmake -S . -B build-gdext -DBUILD_GODOT_EXTENSION=ON
cmake --build build-gdext

# Launch Godot editor with the project
godot --path godot_project/
```

### Unified CI (Phases C+)
- **PR checks**: Compile both Raylib and GDExtension targets; run headless test suite.
- **Parity tests**: Raylib and Godot outputs side-by-side for wave, damage, and movement validation.
- **Network tests**: Cross-version compatibility (Raylib ↔ Godot snapshots).

---

## Scope Boundaries

### What This Document Includes
- High-level architecture recommendation (Godot + GDExtension hybrid).
- Phased migration plan with dependencies and acceptance criteria.
- Subsystem mapping from Raylib to Godot + C++.
- Integration patterns and data contracts.
- Verification checklist and rollback strategy.

### What This Document Excludes
- **Full code implementation** of Godot scenes, scripts, or C++ binding layer (to be done incrementally per phase).
- **Complete GDExtension API reference** (refer to [Godot GDExtension Docs](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html)).
- **Graphics/art assets** (use placeholder models; 3D asset migration is out of scope for Phase 1).
- **Emscripten/Web support** (defer to Phase E; Godot's web export differs from Emscripten build).

---

## Next Steps

1. **Phase A**: Extract engine-agnostic C++ core; establish abstract platform interfaces.
2. **Phase B** (parallel): Create Godot project skeleton and input routing infrastructure.
3. **Phase C**: Implement GDExtension bindings and validate single-player parity.
4. **Phase D**: Wire networking and verify cross-version compatibility.
5. **Phase E**: Android and mobile optimization; performance tuning.

For each phase, create a dedicated Git branch and open a PR with:
- Acceptance criteria checklist (from above).
- Performance benchmarks (FPS, memory, network latency).
- Test coverage (unit tests for game logic, integration tests for scene transitions).

---

## References

- [Godot GDExtension Binding](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html)
- [Godot 4.2 Release Notes](https://docs.godotengine.org/en/stable/about/release_notes.html)
- [Android Port Guide](ANDROID_PORT.md) — existing platform integration patterns
- [Current Game State & Network Protocol](src/main.cpp#L460) — structures to preserve in C++
