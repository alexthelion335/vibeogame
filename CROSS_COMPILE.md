# Cross-Compiling for Windows from Linux

This guide explains how to cross-compile the Chicken Potato FPS game for Windows from a Linux system.

## Prerequisites

Install the MinGW-w64 cross-compiler:

**Ubuntu/Debian:**
```bash
sudo apt install mingw-w64
```

**Arch Linux:**
```bash
sudo pacman -S mingw-w64-gcc
```

## Building

Use the provided CMake toolchain file:

```bash
# Configure
cmake -S . -B build-windows -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-windows -j$(nproc)
```

## Output

The Windows executable will be created at:
```
build-windows/chicken_potato_fps.exe
```

This is a **PE32+ executable** (64-bit Windows) that can be transferred to a Windows machine and run directly.

## Notes

- The executable is statically linked with MinGW runtime libraries for portability
- OpenGL and audio functionality are provided by raylib (included)
- The networking code uses Winsock2, which is available on all Windows systems
- Tested to compile successfully with MinGW-w64 GCC 15.2.0

## Troubleshooting

If you get linker errors about missing libraries, ensure you have the full `mingw-w64` package installed, not just `mingw-w64-gcc`.
