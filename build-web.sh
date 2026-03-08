#!/bin/bash
set -e

# Detect number of CPU cores for parallel compilation
if command -v nproc &> /dev/null; then
    NUM_CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    NUM_CORES=4
fi

echo "Building for HTML5/WebAssembly..."
echo "Using ${NUM_CORES} parallel jobs for compilation"

# Check if emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten is not installed or not in PATH"
    echo "Please install Emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    echo "Or activate it with: source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

echo "Using Emscripten version:"
emcc --version

# Create build directory
mkdir -p build-web

# Configure with Emscripten toolchain
emcmake cmake -S . -B build-web \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Web

# Build
cmake --build build-web --config Release --parallel ${NUM_CORES}

echo ""
echo "Build completed successfully!"
echo "Output files:"
echo "  - build-web/chicken_potato_fps.html"
echo "  - build-web/chicken_potato_fps.js"
echo "  - build-web/chicken_potato_fps.wasm"
echo "  - build-web/chicken_potato_fps.data"
echo ""
echo "To test locally, run:"
echo "  cd build-web && python3 -m http.server 8000"
echo "Then open http://localhost:8000/chicken_potato_fps.html in your browser"
