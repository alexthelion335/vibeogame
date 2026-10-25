#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build-linux-arm64}"
SYSROOT="${ARM64_SYSROOT:-$BUILD_DIR/target-sysroot}"
SYSROOT_LIB_DIR="$SYSROOT/usr/lib/aarch64-linux-gnu"
SYSROOT_IMAGE="${ARM64_SYSROOT_IMAGE:-docker.io/library/debian:trixie-slim}"

for tool in cmake aarch64-linux-gnu-gcc aarch64-linux-gnu-g++; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: required tool '$tool' was not found in PATH." >&2
        exit 1
    fi
done

sysroot_is_ready() {
    [[ -f "$SYSROOT/usr/include/GL/gl.h" &&
       -f "$SYSROOT/usr/include/X11/Xlib.h" &&
       -e "$SYSROOT_LIB_DIR/libX11.so" &&
       -e "$SYSROOT_LIB_DIR/libOpenGL.so" &&
       -e "$SYSROOT_LIB_DIR/libGLX.so" &&
       -e "$SYSROOT_LIB_DIR/libxcb.so.1" &&
       -e "$SYSROOT_LIB_DIR/libGLdispatch.so.0" ]]
}

if ! sysroot_is_ready; then
    if ! command -v podman >/dev/null 2>&1; then
        echo "Error: the arm64 graphics sysroot is incomplete and Podman is unavailable." >&2
        echo "Install Podman, or set ARM64_SYSROOT to a prepared arm64 Linux sysroot." >&2
        exit 1
    fi

    echo "Preparing the arm64 Linux OpenGL/X11 sysroot with Podman..."
    PODMAN_ROOT="${ARM64_PODMAN_ROOT:-${TMPDIR:-/tmp}/vibeogame-podman-root-${UID}}"
    PODMAN_RUNROOT="${ARM64_PODMAN_RUNROOT:-${TMPDIR:-/tmp}/vibeogame-podman-run-${UID}}"
    mkdir -p "$SYSROOT" "$PODMAN_ROOT" "$PODMAN_RUNROOT"

    podman_cmd=(podman --root "$PODMAN_ROOT" --runroot "$PODMAN_RUNROOT" --storage-driver=vfs)
    "${podman_cmd[@]}" run --rm --platform linux/amd64 \
        --volume "$SYSROOT:/out" "$SYSROOT_IMAGE" \
        sh -ec '
            export DEBIAN_FRONTEND=noninteractive
            dpkg --add-architecture arm64
            apt-get update
            apt-get install -y --no-install-recommends \
                libgl-dev:arm64 libglx-dev:arm64 libopengl-dev:arm64 \
                libx11-dev:arm64 libxrandr-dev:arm64 libxi-dev:arm64 \
                libxcursor-dev:arm64 libxinerama-dev:arm64

            mkdir -p /out/usr/include /out/usr/lib/aarch64-linux-gnu
            cp -a /usr/include/GL /usr/include/X11 /out/usr/include/
            cp -a /usr/lib/aarch64-linux-gnu/libGL.so* \
                /usr/lib/aarch64-linux-gnu/libOpenGL.so* \
                /usr/lib/aarch64-linux-gnu/libGLX.so* \
                /usr/lib/aarch64-linux-gnu/libX11.so* \
                /usr/lib/aarch64-linux-gnu/libXrandr.so* \
                /usr/lib/aarch64-linux-gnu/libXi.so* \
                /usr/lib/aarch64-linux-gnu/libXcursor.so* \
                /usr/lib/aarch64-linux-gnu/libXinerama.so* \
                /usr/lib/aarch64-linux-gnu/libXext.so* \
                /usr/lib/aarch64-linux-gnu/libXrender.so* \
                /usr/lib/aarch64-linux-gnu/libXfixes.so* \
                /usr/lib/aarch64-linux-gnu/libXau.so* \
                /usr/lib/aarch64-linux-gnu/libXdmcp.so* \
                /out/usr/lib/aarch64-linux-gnu/
            cp -a /lib/aarch64-linux-gnu/libGLdispatch.so* \
                /lib/aarch64-linux-gnu/libxcb.so* \
                /out/usr/lib/aarch64-linux-gnu/
        '
fi

if ! sysroot_is_ready; then
    echo "Error: sysroot setup completed without the required arm64 GL/X11 files." >&2
    exit 1
fi

if command -v nproc >/dev/null 2>&1; then
    NUM_CORES="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
    NUM_CORES="$(sysctl -n hw.ncpu)"
else
    NUM_CORES=4
fi

echo "Configuring Linux arm64 cross-build..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_FIND_ROOT_PATH="$SYSROOT" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -DCMAKE_EXE_LINKER_FLAGS="-L$SYSROOT_LIB_DIR -Wl,-rpath-link,$SYSROOT_LIB_DIR" \
    -DCMAKE_SKIP_RPATH=ON \
    -DX11_X11_LIB="$SYSROOT_LIB_DIR/libX11.so" \
    -DOPENGL_INCLUDE_DIR="$SYSROOT/usr/include" \
    -DOPENGL_opengl_LIBRARY="$SYSROOT_LIB_DIR/libOpenGL.so" \
    -DOPENGL_glx_LIBRARY="$SYSROOT_LIB_DIR/libGLX.so"

cmake --build "$BUILD_DIR" --parallel "$NUM_CORES"

OUTPUT="$BUILD_DIR/chicken_potato_fps_linux_aarch64"
if [[ ! -f "$OUTPUT" ]]; then
    echo "Error: expected cross-compiled executable was not produced: $OUTPUT" >&2
    exit 1
fi

echo "Linux arm64 build complete: $OUTPUT"
