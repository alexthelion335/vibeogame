echo "Starting build for all platforms..."
echo "WARNING: This script assumes you have the Android SDK, NDK, aapt, zipalign, and apksigner installed and properly configured in your PATH."
echo "DO NOT RUN FOR A FIRST TIME BUILD WITHOUT CHECKING THE CONFIGURATION OF YOUR ANDROID BUILD ENVIRONMENT."
echo "Do you want to continue? (y/n)"
read -r response
if [[ "$response" != "y" ]]; then
    echo "Build cancelled."
    exit 0
fi

# Source Android build variables
set -euo pipefail

# Detect number of CPU cores for parallel compilation
if command -v nproc &> /dev/null; then
    NUM_CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    NUM_CORES=4
fi
echo "Using ${NUM_CORES} parallel jobs for compilation"
export ANDROID_NDK=/opt/android-ndk
export ANDROID_SDK=~/Android/Sdk
export ANDROID_ABI=arm64-v8a  # or armeabi-v7a, x86, x86_64

NATIVE_APP_GLUE="$ANDROID_NDK/sources/android/native_app_glue/android_native_app_glue.c"
if [ ! -f "$NATIVE_APP_GLUE" ]; then
    echo "Error: native_app_glue not found at: $NATIVE_APP_GLUE"
    echo "Set ANDROID_NDK to a valid NDK root (contains sources/android/native_app_glue)."
    exit 1
fi
echo "Building for Windows..."
cmake -S . -B build-windows -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --config Release --parallel ${NUM_CORES}
echo "Building for Linux..."
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --config Release --parallel ${NUM_CORES}
echo "Building for Web/HTML5..."
if command -v emcc &> /dev/null; then
    ./build-web.sh
else
    echo "WARNING: Emscripten not found, skipping web build"
fi
echo "Building for Android..."
./android/build-android.sh
echo "The following commands might ask if you want to overwrite existing files. Please confirm to overwrite if prompted."
cp "build-android/libchicken_potato_fps.so" "apk-build/lib/${ANDROID_ABI}/"
cp android/AndroidManifest.xml apk-build/

# Compile Java activity bridge and generate classes.dex
mkdir -p apk-build/obj
ANDROID_JAR="$ANDROID_SDK/platforms/android-33/android.jar"
BUILD_TOOLS_DIR="$(ls -d "$ANDROID_SDK"/build-tools/* 2>/dev/null | sort -V | tail -n 1)"
if [[ -z "${BUILD_TOOLS_DIR:-}" ]]; then
    echo "Error: No Android build-tools found under $ANDROID_SDK/build-tools"
    exit 1
fi

javac -source 1.8 -target 1.8 \
    -classpath "$ANDROID_JAR" \
    -d apk-build/obj \
    android/src/com/vibeogame/chickenpotato/GameActivity.java

"$BUILD_TOOLS_DIR/d8" \
    --lib "$ANDROID_JAR" \
    --output apk-build \
    apk-build/obj/com/vibeogame/chickenpotato/*.class

cd apk-build
APK_BASE_NAME="chicken_potato_fps-${ANDROID_ABI}"
aapt package -f -M AndroidManifest.xml -I $ANDROID_SDK/platforms/android-33/android.jar -F temp.apk
aapt add temp.apk classes.dex
aapt add temp.apk "lib/${ANDROID_ABI}/libchicken_potato_fps.so"
zipalign -f -v 4 temp.apk "${APK_BASE_NAME}-aligned.apk"
apksigner sign \
   --ks my-release-key.keystore \
   --ks-key-alias my-key-alias \
    --out "${APK_BASE_NAME}.apk" \
    "${APK_BASE_NAME}-aligned.apk"
apksigner verify --verbose "${APK_BASE_NAME}.apk"
cd ..
echo "All builds completed successfully!"
echo "Android APK: apk-build/${APK_BASE_NAME}.apk"