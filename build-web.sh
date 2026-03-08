#!/bin/bash
set -e

echo "Building for HTML5/WebAssembly..."

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
cmake --build build-web --config Release

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
