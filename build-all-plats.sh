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

# echo "Building Linux arm64 (aarch64)..."
# if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 && command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
#     cmake -S . -B build-linux-arm64 \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_SYSTEM_NAME=Linux \
#         -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
#         -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
#         -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
#     cmake --build build-linux-arm64 --parallel "${NUM_CORES}"
#     copy_if_exists "build-linux-arm64/chicken_potato_fps_linux_aarch64"
# else
#     echo "WARNING: aarch64-linux-gnu toolchain not found, skipping Linux arm64 build"
# fi

# echo "Building Windows x86_64..."
# if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
#     cmake -S . -B build-windows \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake
#     cmake --build build-windows --parallel "${NUM_CORES}"
#     copy_if_exists "build-windows/chicken_potato_fps_windows_x86_64.exe"
# else
#     echo "WARNING: MinGW toolchain not found, skipping Windows build"
# fi

# echo "Building Web (Emscripten)..."
# source  /etc/profile.d/emscripten.sh
# if command -v emcc >/dev/null 2>&1; then
#     ./build-web.sh
#     copy_if_exists "build-web/chicken_potato_fps.html"
#     copy_if_exists "build-web/chicken_potato_fps.js"
#     copy_if_exists "build-web/chicken_potato_fps.wasm"
#     copy_if_exists "build-web/chicken_potato_fps.data"
# else
#     echo "WARNING: emcc not found, skipping Web build"
# fi

echo "Building Android native library..."
./android/build-android.sh

echo "Packaging signed Android APK..."
./android/package-apk.sh
copy_if_exists "apk-build/chicken_potato_fps-${ANDROID_ABI}.apk"

echo ""
echo "Release artifacts ready in: ${RELEASE_DIR}"
ls -lah "$RELEASE_DIR"