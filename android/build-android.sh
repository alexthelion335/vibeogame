#!/bin/bash
# Build script for Android using NDK

set -euo pipefail

# Set these paths according to your system
export ANDROID_NDK=/opt/android-ndk
export ANDROID_SDK=~/Android/Sdk
export ANDROID_ABI=arm64-v8a  # or armeabi-v7a, x86, x86_64

NATIVE_APP_GLUE="$ANDROID_NDK/sources/android/native_app_glue/android_native_app_glue.c"
if [ ! -f "$NATIVE_APP_GLUE" ]; then
    echo "Error: native_app_glue not found at: $NATIVE_APP_GLUE"
    echo "Set ANDROID_NDK to a valid NDK root (contains sources/android/native_app_glue)."
    exit 1
fi

# Create build directory
mkdir -p build-android
cd build-android

# Remove stale Android cache entries that can keep a bad NDK path
if [ -f CMakeCache.txt ]; then
    rm -f CMakeCache.txt
fi

# Configure with CMake
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=21 \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_ANDROID_NDK=${ANDROID_NDK} \
    -DANDROID_NDK=${ANDROID_NDK} \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID=ON

# Build
cmake --build . -j$(nproc)

echo "Build complete. Library at: build-android/libchicken_potato_fps.so"
