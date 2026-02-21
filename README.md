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

### Requirements

- CMake 3.16+
- A C++17 compiler
- OpenGL development libraries (Linux)

### Ubuntu / Debian

Install dependencies:
```bash
sudo apt update
sudo apt install build-essential cmake libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

Build and run:
```bash
cmake -S . -B build && cmake --build build -j$(nproc)
./build/chicken_potato_fps
```

### Arch Linux

Install dependencies:
```bash
sudo pacman -S base-devel cmake mesa libx11 libxrandr libxi libxcursor libxinerama
```

Build and run:
```bash
cmake -S . -B build && cmake --build build -j$(nproc)
./build/chicken_potato_fps
```

### Windows

Install dependencies:
- [CMake](https://cmake.org/download/) (add to PATH during installation)
- [Visual Studio 2019 or later](https://visualstudio.microsoft.com/) with C++ desktop development workload
- Or [MinGW-w64](https://www.mingw-w64.org/) if using GCC

Build and run (Visual Studio):
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\chicken_potato_fps.exe
```

Build and run (MinGW):
```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\chicken_potato_fps.exe
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
