#!/bin/bash
set -euo pipefail

echo "Starting multi-platform release build..."

if command -v nproc >/dev/null 2>&1; then
    NUM_CORES=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    NUM_CORES=4
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"
RELEASE_DIR="release-artifacts"

mkdir -p "$RELEASE_DIR"
rm -f "$RELEASE_DIR"/*

copy_if_exists() {
    local src="$1"
    if [[ -f "$src" ]]; then
        cp "$src" "$RELEASE_DIR/"
        echo "  + $(basename "$src")"
    fi
}

echo "Using ${NUM_CORES} parallel jobs"

echo "Building Linux x86_64..."
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel "${NUM_CORES}"
copy_if_exists "build-linux/chicken_potato_fps_linux_x86_64"

echo "Building Linux arm64 (aarch64)..."
if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 && command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
    ARM64_SYSROOT_PATH="${ARM64_SYSROOT:-build-linux-arm64/target-sysroot}"
    if [[ -f "$ARM64_SYSROOT_PATH/usr/include/GL/gl.h" ]] || command -v podman >/dev/null 2>&1; then
        ./build-linux-arm64.sh
        copy_if_exists "build-linux-arm64/chicken_potato_fps_linux_aarch64"
    else
        echo "WARNING: arm64 OpenGL/X11 sysroot unavailable and Podman not found, skipping Linux arm64 build"
    fi
else
    echo "WARNING: aarch64-linux-gnu toolchain not found, skipping Linux arm64 build"
fi

echo "Building Windows x86_64..."
if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    cmake -S . -B build-windows \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake
    cmake --build build-windows --parallel "${NUM_CORES}"
    copy_if_exists "build-windows/chicken_potato_fps_windows_x86_64.exe"
else
    echo "WARNING: MinGW toolchain not found, skipping Windows build"
fi

echo "Building Web (Emscripten)..."
if ! command -v emcc >/dev/null 2>&1 && [[ -f /etc/profile.d/emscripten.sh ]]; then
    # Load the system Emscripten environment when the script was not launched
    # from a shell where emsdk is already activated.
    source /etc/profile.d/emscripten.sh
fi
if command -v emcc >/dev/null 2>&1; then
    ./build-web.sh
    for web_html in build-web/chicken_potato_fps_web_*.html; do
        [[ -f "$web_html" ]] || continue
        web_basename="${web_html%.html}"
        copy_if_exists "${web_basename}.html"
        copy_if_exists "${web_basename}.js"
        copy_if_exists "${web_basename}.wasm"
        copy_if_exists "${web_basename}.data"
    done
else
    echo "WARNING: emcc not found, skipping Web build"
fi

echo "Building Android native library..."
./android/build-android.sh

echo "Packaging signed Android APK..."
./android/package-apk.sh
copy_if_exists "apk-build/chicken_potato_fps-${ANDROID_ABI}.apk"

echo ""
echo "Release artifacts ready in: ${RELEASE_DIR}"
ls -lah "$RELEASE_DIR"
