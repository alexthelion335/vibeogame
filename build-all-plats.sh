echo "Starting build for all platforms..."
echo "WARNING: This script assumes you have the Android SDK, NDK, aapt, zipalign, and apksigner installed and properly configured in your PATH."
echo "DO NOT RUN FOR A FIRST TIME BUILD WITHOUT CHECKING THE CONFIGURATION OF YOUR ANDROID BUILD ENVIRONMENT."
echo "Do you want to continue? (y/n)"
read -r response
if [[ "$response" != "y" ]]; then
    echo "Build cancelled."
    exit 0
fi
echo "Building for Windows..."
cmake -S . -B build-windows -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --config Release
echo "Building for Linux..."
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --config Release
echo "Building for Android..."
./android/build-android.sh
cp build-android/libchicken_potato_fps.so apk-build/lib/arm64-v8a/
cp android/AndroidManifest.xml apk-build/
cd apk-build
aapt package -f -M AndroidManifest.xml -I $ANDROID_SDK/platforms/android-33/android.jar -F temp.apk
aapt add temp.apk lib/arm64-v8a/libchicken_potato_fps.so
zipalign -f -v 4 temp.apk chicken_potato_fps-aligned.apk
apksigner sign \
   --ks my-release-key.keystore \
   --ks-key-alias my-key-alias \
   --out chicken_potato_fps.apk \
   chicken_potato_fps-aligned.apk
apksigner verify --verbose chicken_potato_fps.apk
cd ..
echo "All builds completed successfully!"