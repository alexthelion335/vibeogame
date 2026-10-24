#!/bin/bash
set -euo pipefail

# Simple signed-APK packager for Chicken Potato FPS.
# Expects native library from ./android/build-android.sh at build-android/libchicken_potato_fps.so

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

ANDROID_SDK="${ANDROID_SDK:-$HOME/Android/Sdk}"
ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"
ANDROID_API="${ANDROID_API:-33}"
ANDROID_MIN_SDK="${ANDROID_MIN_SDK:-21}"
APP_VERSION_CODE="${APP_VERSION_CODE:-3}"
APP_VERSION_NAME="${APP_VERSION_NAME:-1.3.0}"

KEYSTORE_PATH="${KEYSTORE_PATH:-$ROOT_DIR/android/my-release-key.keystore}"
KEY_ALIAS="${KEY_ALIAS:-my-key-alias}"
KEYSTORE_PASS="${KEYSTORE_PASS:-}"
KEY_PASS="${KEY_PASS:-$KEYSTORE_PASS}"

if [[ ! -f "build-android/libchicken_potato_fps.so" ]]; then
    echo "Error: build-android/libchicken_potato_fps.so not found."
    echo "Run ./android/build-android.sh first."
    exit 1
fi

if [[ ! -d "$ANDROID_SDK" ]]; then
    echo "Error: ANDROID_SDK not found at $ANDROID_SDK"
    exit 1
fi

if [[ ! -f "$KEYSTORE_PATH" ]]; then
    echo "Error: keystore not found at $KEYSTORE_PATH"
    echo "Create one with:"
    echo "  keytool -genkey -v -keystore $KEYSTORE_PATH -alias $KEY_ALIAS -keyalg RSA -keysize 2048 -validity 10000"
    exit 1
fi

for cmd in javac aapt zipalign apksigner; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Error: required tool '$cmd' is not in PATH"
        exit 1
    fi
done

ANDROID_JAR="$ANDROID_SDK/platforms/android-${ANDROID_API}/android.jar"
if [[ ! -f "$ANDROID_JAR" ]]; then
    echo "Error: $ANDROID_JAR not found"
    exit 1
fi

BUILD_TOOLS_DIR="$(ls -d "$ANDROID_SDK"/build-tools/* 2>/dev/null | sort -V | tail -n 1)"
if [[ -z "${BUILD_TOOLS_DIR:-}" ]]; then
    echo "Error: no build-tools found under $ANDROID_SDK/build-tools"
    exit 1
fi

D8_BIN="$BUILD_TOOLS_DIR/d8"
if [[ ! -x "$D8_BIN" ]]; then
    echo "Error: d8 not found at $D8_BIN"
    exit 1
fi

APK_BUILD_DIR="$ROOT_DIR/apk-build"
rm -rf "$APK_BUILD_DIR"
mkdir -p "$APK_BUILD_DIR/lib/$ANDROID_ABI" "$APK_BUILD_DIR/res" "$APK_BUILD_DIR/obj"

cp build-android/libchicken_potato_fps.so "$APK_BUILD_DIR/lib/$ANDROID_ABI/"
cp android/AndroidManifest.xml "$APK_BUILD_DIR/"
cp -r android/res/. "$APK_BUILD_DIR/res/"

javac -source 1.8 -target 1.8 \
    -classpath "$ANDROID_JAR" \
    -d "$APK_BUILD_DIR/obj" \
    android/src/com/alexkinch/chickenpotato/GameActivity.java

"$D8_BIN" \
    --lib "$ANDROID_JAR" \
    --output "$APK_BUILD_DIR" \
    "$APK_BUILD_DIR"/obj/com/alexkinch/chickenpotato/*.class

pushd "$APK_BUILD_DIR" >/dev/null

APK_BASE_NAME="chicken_potato_fps-${ANDROID_ABI}"
aapt package -f \
    -M AndroidManifest.xml \
    -S res \
    -I "$ANDROID_JAR" \
    --min-sdk-version "$ANDROID_MIN_SDK" \
    --target-sdk-version "$ANDROID_API" \
    --version-code "$APP_VERSION_CODE" \
    --version-name "$APP_VERSION_NAME" \
    --replace-version \
    --error-on-failed-insert \
    -F temp.apk
aapt add temp.apk classes.dex
aapt add temp.apk "lib/${ANDROID_ABI}/libchicken_potato_fps.so"

zipalign -f -v 4 temp.apk "${APK_BASE_NAME}-aligned.apk"

if [[ -n "$KEYSTORE_PASS" ]]; then
    apksigner sign \
        --ks "$KEYSTORE_PATH" \
        --ks-key-alias "$KEY_ALIAS" \
        --ks-pass "pass:$KEYSTORE_PASS" \
        --key-pass "pass:${KEY_PASS:-$KEYSTORE_PASS}" \
        --out "${APK_BASE_NAME}.apk" \
        "${APK_BASE_NAME}-aligned.apk"
else
    apksigner sign \
        --ks "$KEYSTORE_PATH" \
        --ks-key-alias "$KEY_ALIAS" \
        --out "${APK_BASE_NAME}.apk" \
        "${APK_BASE_NAME}-aligned.apk"
fi

apksigner verify --verbose "${APK_BASE_NAME}.apk"

popd >/dev/null

echo "Signed APK created: $APK_BUILD_DIR/${APK_BASE_NAME}.apk"
