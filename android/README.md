# Android Build Instructions for Chicken Potato FPS

## Prerequisites

1. **Android NDK** (r21 or later): Download from https://developer.android.com/ndk/downloads
2. **Android SDK** (with platform-tools): Download from https://developer.android.com/studio
3. **CMake** (3.16 or later)

## Building

### Option 1: Using the build script (recommended)

1. Edit `android/build-android.sh` and set the correct paths:
   - `ANDROID_NDK`: Path to your Android NDK installation
   - `ANDROID_SDK`: Path to your Android SDK installation
   - `ANDROID_ABI`: Target architecture (arm64-v8a, armeabi-v7a, x86, x86_64)

2. Make the script executable and run it:
   ```bash
   chmod +x android/build-android.sh
   ./android/build-android.sh
   ```

### Option 2: Manual CMake build

```bash
mkdir -p build-android
cd build-android

cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=21 \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK=/path/to/android-ndk \
   -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID=ON

cmake --build . -j$(nproc)
```

## Creating the APK

After building, you'll have `libchicken_potato_fps.so`. To create a full APK:

> Important: If you built with `c++_shared` instead of `c++_static`, you must also package `libc++_shared.so` for the same ABI or the app will crash on launch.

### Using Android Studio (easier):

1. Create a new Native C++ project in Android Studio
2. Copy `android/AndroidManifest.xml` to your project
3. Copy the built `.so` file to `app/src/main/jniLibs/<abi>/`
4. Build APK from Android Studio

### Using command line tools:

```bash
# Set up project structure
mkdir -p apk-build/lib/arm64-v8a
cp build-android/libchicken_potato_fps.so apk-build/lib/arm64-v8a/
cp android/AndroidManifest.xml apk-build/

# Only needed if built with -DCMAKE_ANDROID_STL_TYPE=c++_shared
# cp /path/to/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so apk-build/lib/arm64-v8a/

# Create unsigned APK (manifest/resources only)
cd apk-build
aapt package -f -M AndroidManifest.xml -I $ANDROID_SDK/platforms/android-33/android.jar -F temp.apk

# Add native libraries under lib/<abi>/ so NativeActivity can load them
aapt add temp.apk lib/arm64-v8a/libchicken_potato_fps.so
# If built with c++_shared, also add:
# aapt add temp.apk lib/arm64-v8a/libc++_shared.so

# Align unsigned APK first
zipalign -f -v 4 temp.apk chicken_potato_fps-aligned.apk

# Create keystore once (if needed)
keytool -genkey -v -keystore my-release-key.keystore -alias my-key-alias -keyalg RSA -keysize 2048 -validity 10000

# Sign with APK Signature Scheme v2/v3 (required by modern Android)
apksigner sign \
   --ks my-release-key.keystore \
   --ks-key-alias my-key-alias \
   --out chicken_potato_fps.apk \
   chicken_potato_fps-aligned.apk

# Verify signature
apksigner verify --verbose chicken_potato_fps.apk
```

## Installing on Device

```bash
adb install -r chicken_potato_fps.apk
```

## Supported Android Versions

- **Minimum SDK**: Android 5.0 (API 21)
- **Target SDK**: Android 13 (API 33)
- **Architectures**: arm64-v8a, armeabi-v7a, x86_64, x86

## Notes

- The game runs in landscape mode by default
- Touch controls are automatically enabled on Android
- Network permissions are included for multiplayer support
- The app requires OpenGL ES 2.0 or higher
