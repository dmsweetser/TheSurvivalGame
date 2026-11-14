#!/bin/bash
# Build TheSurvivalGame.apk: a thin WebView wrapper around the single-file
# web build (the-survival-game.html). Manual aapt2/javac/d8 pipeline — no
# Gradle. Needs: JDK (keytool/javac) + Android SDK build-tools 34 and
# platform android-34 under ~/workspace/tools (see /tmp/android-setup.sh).
set -e
cd "$(dirname "$0")"

export JAVA_HOME="${JAVA_HOME:-$HOME/workspace/tools/jdk17}"
export PATH="$JAVA_HOME/bin:$PATH"
ANDROID_HOME="${ANDROID_HOME:-$HOME/workspace/tools/android-sdk}"
BT="$ANDROID_HOME/build-tools/34.0.0"
AJAR="$ANDROID_HOME/platforms/android-34/android.jar"
GAME_HTML="$HOME/workspace/goals/the-survival-game/files/the-survival-game.html"
OUT_DIR="$HOME/workspace/goals/the-survival-game/files"
KEYSTORE="$HOME/workspace/survival-game/android/debug.keystore"

[ -f "$GAME_HTML" ] || { echo "missing game html: $GAME_HTML (run build.py first)"; exit 1; }
[ -x "$BT/aapt2" ] || { echo "missing Android build-tools at $BT"; exit 1; }

rm -rf build && mkdir -p build/classes build/dex build/stage/assets
cp "$GAME_HTML" build/stage/assets/index.html

# version identity comes from android/VERSION — one source of truth,
# shared with the Play build so the flavors can never drift apart
VER_CODE="$(sed -n 's/^CODE=//p' VERSION)"
VER_NAME="$(sed -n 's/^NAME=//p' VERSION)"
[ -n "$VER_CODE" ] && [ -n "$VER_NAME" ] || { echo "android/VERSION is unreadable"; exit 1; }

# itch-flavor manifest: shared manifest + version stamp + INTERNET
# (update notice only — the app never installs APKs itself; that
# machinery is what Play Protect flagged in 1.1). The Play build
# (build_aab.sh) generates its own play-flavor manifest from the
# same source and stays at ZERO permissions.
python3 flavor_manifest.py AndroidManifest.xml build/AndroidManifest-itch.xml itch "$VER_CODE" "$VER_NAME"

# resources + manifest -> base apk
"$BT/aapt2" compile --dir res -o build/res.zip
"$BT/aapt2" link -o build/base.apk -I "$AJAR" --manifest build/AndroidManifest-itch.xml \
    --java build/gen build/res.zip

# java -> dex (shared sources + the ITCH flavor's notice-only Updater)
"$JAVA_HOME/bin/javac" -encoding UTF-8 -cp "$AJAR" -d build/classes \
    src/com/dmsweetser/survivalgame/MainActivity.java \
    src/com/dmsweetser/survivalgame/ShareProvider.java \
    src-itch/com/dmsweetser/survivalgame/Updater.java \
    build/gen/com/dmsweetser/survivalgame/R.java
"$BT/d8" --lib "$AJAR" --output build/dex $(find build/classes -name '*.class')

# assemble: base apk + classes.dex + assets/index.html
cp build/base.apk build/unsigned.apk
cp build/dex/classes.dex build/stage/classes.dex
(cd build/stage && zip -q -r ../unsigned.apk classes.dex assets)

"$BT/zipalign" -f 4 build/unsigned.apk build/aligned.apk

if [ ! -f "$KEYSTORE" ]; then
    "$JAVA_HOME/bin/keytool" -genkeypair -keystore "$KEYSTORE" -storepass android \
        -keypass android -alias androiddebugkey -keyalg RSA -keysize 2048 \
        -validity 10000 -dname "CN=The Survival Game (debug), O=dmsweetser" > /dev/null 2>&1
fi
"$BT/apksigner" sign --ks "$KEYSTORE" --ks-pass pass:android --key-pass pass:android \
    --out "$OUT_DIR/TheSurvivalGame.apk" build/aligned.apk

"$BT/apksigner" verify "$OUT_DIR/TheSurvivalGame.apk"
"$BT/aapt2" dump badging "$OUT_DIR/TheSurvivalGame.apk" | head -4
ls -la "$OUT_DIR/TheSurvivalGame.apk"
echo "APK OK -> $OUT_DIR/TheSurvivalGame.apk"
