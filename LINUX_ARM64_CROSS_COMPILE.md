# Linux arm64 cross-build

The Linux arm64 build uses the `aarch64-linux-gnu` GCC cross-compiler and an
arm64 OpenGL/X11 sysroot. The build script creates the graphics part of that
sysroot with Podman and Debian's arm64 development packages, then configures
CMake to search that sysroot for target headers and libraries. It never links
the host's x86_64 OpenGL or X11 libraries.

## Prerequisites

- CMake and Bash
- `aarch64-linux-gnu-gcc` and `aarch64-linux-gnu-g++`
- Podman (only needed if the sysroot does not already exist)
- Network access for Podman to pull the Debian image and download arm64 packages

On Arch Linux, the cross-compiler is provided by `aarch64-linux-gnu-gcc`.
Install the host tools from the regular system repositories if needed:

```bash
sudo pacman -S --needed cmake aarch64-linux-gnu-gcc podman
```

## Build

Run the dedicated script:

```bash
./build-linux-arm64.sh
```

The script prepares `build-linux-arm64/target-sysroot` if needed, configures
the `build-linux-arm64` CMake tree, and builds the executable:

```text
build-linux-arm64/chicken_potato_fps_linux_aarch64
```

It uses Debian Trixie's arm64 GLVND/OpenGL and X11 development packages. The
result links dynamically to the target's `libX11.so.6`, `libOpenGL.so.0`, and
`libGLX.so.0`; those runtime libraries must be available on the arm64 machine.
The executable has no RPATH pointing back to the build host. With the current
Arch cross-compiler, it also requires glibc 2.43 or newer; to target an older
arm64 distribution, use a cross-toolchain and C runtime sysroot matching that
distribution.

To use a pre-existing compatible sysroot, set `ARM64_SYSROOT`:

```bash
ARM64_SYSROOT=/path/to/arm64-sysroot ./build-linux-arm64.sh
```

The sysroot must provide `usr/include/GL/gl.h`, `usr/include/X11/Xlib.h`, and
the arm64 X11/OpenGL libraries checked by the script. Set
`ARM64_SYSROOT_IMAGE` to override the Debian container image used for sysroot
setup.

## Build with all release platforms

`build-all-plats.sh` calls `build-linux-arm64.sh` when the cross-compiler is
installed and either a prepared sysroot or Podman is available. On success it
copies the arm64 executable into `release-artifacts/` alongside the other
platform outputs.

## Troubleshooting

- If CMake reports missing `GL/gl.h` or X11, verify Podman completed sysroot
  setup and that `build-linux-arm64/target-sysroot` contains the GL, X11, and
  arm64 libraries.
- If the executable fails on the target with a missing shared library, install
  the matching arm64 runtime package for the target distribution.
- `ARM64_SYSROOT` can point to an existing target sysroot to avoid Podman.
