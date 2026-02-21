# Chicken Potato FPS (C++ / raylib)

A small 3D first-person survival shooter where you fight off hordes of angry chickens with a potato cannon.

## Features

- First-person movement (`WASD`) + mouse look
- Potato cannon projectile shooting
- Chicken horde AI that chases player
- Wave-based difficulty scaling
- Local co-op mode
- Online multiplayer mode (host/join by IP, LAN/port-forward)
- Health, score, wave UI
- Restart on death

## Build

Requirements:

- CMake 3.16+
- A C++17 compiler
- OpenGL development libraries (Linux)

Build and run:

```bash
cmake -S . -B build
cmake --build build -j
./build/chicken_potato_fps
```

## Controls

- `W A S D` - move
- `Mouse` - look
- `Left Click` - fire potato
- `Space` - jump
- `Left Shift` - sprint
- `R` - restart when dead
- `Esc` - pause menu

### Start menu modes

- `Singleplayer`
- `Co-op` (local second player): `I J K L` move, `Right Shift` shoot, `Right Ctrl` sprint
- `Online Multiplayer`: choose `Host Game` or `Join Game` and enter host IPv4 (default `127.0.0.1`)

Online mode uses UDP port `42069`.
