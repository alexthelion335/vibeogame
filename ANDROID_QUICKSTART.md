# Quick Start: Building for Android

## Prerequisites

Download and install:
1. **Android NDK** r21 or later: https://developer.android.com/ndk/downloads
2. **Android SDK** with platform-tools: https://developer.android.com/studio
3. **CMake** 3.16+: Should already be installed if you built the desktop version

## Step 1: Configure Build Script

Edit `android/build-android.sh` and set these paths for your system:

```bash
export ANDROID_NDK=/path/to/android-ndk       # e.g., ~/Android/Sdk/ndk/25.1.8937393
export ANDROID_SDK=/path/to/android-sdk       # e.g., ~/Android/Sdk
export ANDROID_ABI=arm64-v8a                  # or armeabi-v7a for older devices
```

## Step 2: Build

```bash
cd /home/alex/Repos/vibeogame
chmod +x android/build-android.sh
./android/build-android.sh
```

This will create: `build-android/libchicken_potato_fps.so`

Note: The build script uses `c++_static` to avoid missing `libc++_shared.so` crashes at startup.

## Step 3: Create APK (Choose One Method)

### Method A: Using Android Studio (Easier)

1. Open Android Studio
2. Create new "Native C++" project
3. Copy files:
   - `android/AndroidManifest.xml` → `app/src/main/`
   - `build-android/libchicken_potato_fps.so` → `app/src/main/jniLibs/arm64-v8a/`
   - If you built manually with `c++_shared`, also copy `libc++_shared.so` into the same `jniLibs/arm64-v8a/` folder.
4. Build → Generate Signed Bundle / APK
5. Follow the wizard to sign and build

### Method B: Command Line (Advanced)

See full instructions in `android/README.md`

## Step 4: Install on Device

```bash
# Enable USB debugging on your Android device first!
adb install -r your_app.apk
```

## Building Without Android Studio

**You don't need Android Studio!** The project is set up to build with just CMake and the NDK. Android Studio is only needed if you want to:
- Package the APK easily
- Debug on device
- Add Java/Kotlin code
- Use the visual editor

For pure C++ development, the CMake + NDK approach is sufficient.

## Testing on Emulator

1. Create an AVD in Android Studio with:
   - Android 5.0+ (API 21+)
   - ABI matching your build (arm64-v8a or x86_64)
   - OpenGL ES 2.0 support
2. Start the emulator
3. Install APK with `adb install`

## Troubleshooting

### Build fails with "NDK not found"
- Check NDK path in build-android.sh
- Verify NDK is extracted/installed correctly
- Try absolute paths instead of relative

### "Permission denied" on build script
```bash
chmod +x android/build-android.sh
```

### APK won't install
- Check device has "Unknown sources" enabled (Android < 8)
- Check "Install unknown apps" permission (Android 8+)
- Try `adb uninstall com.vibeogame.chickenpotato` first
- Check for signature conflicts if reinstalling
- If you see `INSTALL_PARSE_FAILED_NO_CERTIFICATES`, sign with `apksigner` (not only `jarsigner`) so the APK has v2/v3 signatures

### Touch controls not showing
- Make sure PLATFORM_ANDROID is defined (automatic in CMake)
- Check that game is in Playing state (not menu)
- Verify screen isn't paused

### Game crashes on start
- Check logcat: `adb logcat | grep chicken`
- Verify OpenGL ES 2.0 support on device
- Check if device meets minimum API 21 requirement
- If logcat says `Unable to find native library chicken_potato_fps`, your APK likely has `arm64-v8a/libchicken_potato_fps.so` instead of `lib/arm64-v8a/libchicken_potato_fps.so`
- Rebuild APK by running `aapt package ... -F temp.apk` first, then `aapt add temp.apk lib/arm64-v8a/libchicken_potato_fps.so`

### Controls not responding
- Touch might be captured by UI elements
- Check button collision rectangles aren't overlapping
- Verify multi-touch support on device
- Try restarting the app

## Architecture Support

Build for multiple architectures:

```bash
# For modern 64-bit devices (recommended)
export ANDROID_ABI=arm64-v8a

# For older 32-bit devices
export ANDROID_ABI=armeabi-v7a

# For Intel devices (rare)
export ANDROID_ABI=x86_64
```

## What Works on Android

✅ Touch controls (joystick + buttons)
✅ Touch camera rotation (drag to look)
✅ All weapons and gameplay
✅ Singleplayer mode
✅ Local co-op (needs 2 touch input sets - future enhancement)
✅ Online multiplayer (with INTERNET permission)
✅ All menus and UI
✅ Save/load (uses Android internal storage)
✅ DPI scaling (automatic)
✅ Landscape orientation

## What's Different on Android

- No keyboard/mouse support (all touch)
- No window resizing (always fullscreen)
- No sprint button (could be added as button or gesture)
- Weapon switching cycles instead of direct select
- Cursor always visible (for UI interaction)
- Performance may vary by device

## Need Help?

See `ANDROID_PORT.md` for complete technical documentation.
