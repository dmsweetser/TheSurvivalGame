package com.dmsweetser.survivalgame;

import android.app.Activity;

// PLAY FLAVOR ONLY — a deliberate no-op. Google Play delivers updates
// to this build itself (auto-update is the Play default), and Play
// policy forbids apps that fetch and install APKs around Play's own
// update mechanism. The real update check lives in src-itch and is
// compiled ONLY into the itch/direct-download APK (build_apk.sh);
// this stub keeps MainActivity's one call site identical across
// flavors without putting any self-update code in the Play build.
public class Updater {
    public static void maybeCheck(Activity act) {
        // Play handles updates. Nothing to do, ever.
    }
}
