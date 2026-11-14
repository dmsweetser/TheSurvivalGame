#!/bin/bash
# Build TheSurvivalGame.aab: a signed Android App Bundle for Google Play,
# from the same sources/assets as build_apk.sh. Manual aapt2/javac/d8 +
# bundletool pipeline — no Gradle.
#
# Differences from the APK build (build_apk.sh is NOT modified and its
# flow is untouched):
#   - targetSdk 36 (Android 16) — Play requires API 36 for new apps since
#     Aug 31, 2026. The shared AndroidManifest.xml keeps targetSdk 34 for
#     the APK; this script generates a target-36 manifest variant in its
#     own build dir and links that.
#   - Resources are linked in aapt2's proto format and assembled into a
#     bundletool base-module zip instead of a plain APK zip.
#   - The bundle is signed with jarsigner (apksigner does not sign AABs),
#     using the same shared debug.keystore as the APK build. NOTE: for a
#     real Play submission this must be a proper upload key enrolled in
#     Play App Signing — see the handover notes.
#
# Needs: JDK 17, Android SDK build-tools 34.0.0, platform android-36
# (~/workspace/tools/android-sdk), and bundletool
# (~/workspace/tools/bundletool-all-1.18.3.jar, official google/bundletool
# GitHub release).
set -e
cd "$(dirname "$0")"

export JAVA_HOME="${JAVA_HOME:-$HOME/workspace/tools/jdk17}"
export PATH="$JAVA_HOME/bin:$PATH"
ANDROID_HOME="${ANDROID_HOME:-$HOME/workspace/tools/android-sdk}"
BT="$ANDROID_HOME/build-tools/34.0.0"
AJAR36="$ANDROID_HOME/platforms/android-36/android.jar"
BUNDLETOOL="$HOME/workspace/tools/bundletool-all-1.18.3.jar"
GAME_HTML="$HOME/workspace/goals/the-survival-game/files/the-survival-game.html"
OUT_DIR="$HOME/workspace/goals/the-survival-game/files"
KEYSTORE="$HOME/workspace/survival-game/android/debug.keystore"
BUILD=build-aab

[ -f "$GAME_HTML" ] || { echo "missing game html: $GAME_HTML (run build.py first)"; exit 1; }
[ -x "$BT/aapt2" ] || { echo "missing Android build-tools at $BT"; exit 1; }
[ -f "$AJAR36" ] || { echo "missing android-36 platform at $AJAR36"; exit 1; }
[ -f "$BUNDLETOOL" ] || { echo "missing bundletool at $BUNDLETOOL"; exit 1; }
[ -f "$KEYSTORE" ] || { echo "missing keystore $KEYSTORE (run build_apk.sh once)"; exit 1; }

rm -rf "$BUILD" && mkdir -p "$BUILD/classes" "$BUILD/dex" "$BUILD/gen" "$BUILD/module"

# --- manifest variant: play flavor (version stamp from
# android/VERSION, shared with build_apk.sh; NO update permissions —
# Play updates this build itself), then targetSdk 34 -> 36 ---
VER_CODE="$(sed -n 's/^CODE=//p' VERSION)"
VER_NAME="$(sed -n 's/^NAME=//p' VERSION)"
[ -n "$VER_CODE" ] && [ -n "$VER_NAME" ] || { echo "android/VERSION is unreadable"; exit 1; }
python3 flavor_manifest.py AndroidManifest.xml "$BUILD/AndroidManifest-play.xml" play "$VER_CODE" "$VER_NAME"
sed 's/android:targetSdkVersion="34"/android:targetSdkVersion="36"/' \
    "$BUILD/AndroidManifest-play.xml" > "$BUILD/AndroidManifest.xml"
grep -q 'targetSdkVersion="36"' "$BUILD/AndroidManifest.xml" || {
    echo "targetSdk bump did not apply — manifest format changed?"; exit 1; }
grep -q 'android.permission' "$BUILD/AndroidManifest.xml" && {
    echo "PLAY MANIFEST GREW A PERMISSION — the flavor split broke"; exit 1; }

# --- resources: compile, then link in proto format for the bundle ---
"$BT/aapt2" compile --dir res -o "$BUILD/res.zip"
"$BT/aapt2" link --proto-format -o "$BUILD/linked.ap_" -I "$AJAR36" \
    --manifest "$BUILD/AndroidManifest.xml" \
    --java "$BUILD/gen" "$BUILD/res.zip"

# --- java -> dex (compiled against the API 36 platform jar) ---
"$JAVA_HOME/bin/javac" -encoding UTF-8 -cp "$AJAR36" -d "$BUILD/classes" \
    src/com/dmsweetser/survivalgame/MainActivity.java \
    src/com/dmsweetser/survivalgame/ShareProvider.java \
    src-play/com/dmsweetser/survivalgame/Updater.java \
    "$BUILD/gen/com/dmsweetser/survivalgame/R.java"
"$BT/d8" --lib "$AJAR36" --output "$BUILD/dex" $(find "$BUILD/classes" -name '*.class')

# --- assemble the base-module zip bundletool expects ---
# layout: manifest/AndroidManifest.xml (proto), resources.pb, res/**,
#         dex/classes.dex, assets/**
(cd "$BUILD/module" && unzip -q "../linked.ap_")
mkdir -p "$BUILD/module/manifest" "$BUILD/module/dex" "$BUILD/module/assets"
mv "$BUILD/module/AndroidManifest.xml" "$BUILD/module/manifest/AndroidManifest.xml"
cp "$BUILD/dex/classes.dex" "$BUILD/module/dex/classes.dex"
cp "$GAME_HTML" "$BUILD/module/assets/index.html"
(cd "$BUILD/module" && zip -q -r ../base.zip manifest resources.pb res dex assets)

# --- build the bundle, then sign it with the shared keystore ---
"$JAVA_HOME/bin/java" -jar "$BUNDLETOOL" build-bundle \
    --modules="$BUILD/base.zip" --output="$BUILD/unsigned.aab"
"$JAVA_HOME/bin/jarsigner" -keystore "$KEYSTORE" -storepass android \
    -keypass android -sigalg SHA256withRSA -digestalg SHA-256 \
    -signedjar "$OUT_DIR/TheSurvivalGame.aab" "$BUILD/unsigned.aab" androiddebugkey

# --- verify ---
"$JAVA_HOME/bin/java" -jar "$BUNDLETOOL" validate --bundle="$OUT_DIR/TheSurvivalGame.aab"
ls -la "$OUT_DIR/TheSurvivalGame.aab"
echo "AAB OK -> $OUT_DIR/TheSurvivalGame.aab"
