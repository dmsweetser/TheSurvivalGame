package com.dmsweetser.survivalgame;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.net.Uri;
import android.os.Build;

import org.json.JSONObject;

import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

// ITCH FLAVOR ONLY — the launch-time update NOTICE for the sideloaded
// (itch.io / direct-download) build. The Play build compiles the no-op
// stub in src-play instead: Google Play updates its own apps.
//
// What this does, deliberately and nothing more: once per launch it
// fetches a tiny JSON manifest from the project's own version worker.
// If the manifest's versionCode beats the installed one, the player
// gets a prompt — "A NEW BUILD IS OUT" — and one tap opens the game's
// itch page IN THE BROWSER. From there the player downloads the new
// APK and installs it exactly like the first install; saves carry
// over.
//
// Why the browser handoff (changed in 1.2): the 1.1 build downloaded
// the APK in-app and handed it to the system installer itself, which
// needed REQUEST_INSTALL_PACKAGES. That fetch-config → download-APK →
// install chain is precisely the dropper pattern Google Play Protect
// heuristics flag as a "harmful app", and Protect started flagging
// 1.1 within hours of publish (1.0, with no updater at all, was never
// flagged; both were signed with the same key). So the app no longer
// downloads or installs anything — it only notices and points. This
// build requests INTERNET and nothing else.
//
// Every failure — offline, server down, bad JSON — is silent; the
// game just plays. The check sends nothing: a plain GET of a static
// file, no identifiers, no analytics.
//
// Fire OS note: AlertDialog/HttpURLConnection and an https ACTION_VIEW
// are stock Android and work on Kindle Fire the same way (Silk or any
// installed browser opens the page).
public class Updater {
    private static final String VERSION_URL = "https://tsg-version.dmsweetser.workers.dev/version.json";
    private static boolean checked = false;

    public static void maybeCheck(final Activity act) {
        if (checked) return;
        checked = true;
        new Thread(() -> {
            try {
                HttpURLConnection c = (HttpURLConnection) new URL(VERSION_URL).openConnection();
                c.setConnectTimeout(5000);
                c.setReadTimeout(5000);
                c.setRequestProperty("Accept", "application/json");
                if (c.getResponseCode() != 200) return;
                String body = readAll(c.getInputStream());
                JSONObject j = new JSONObject(body);
                int latest = j.getInt("versionCode");
                if (latest <= installedCode(act)) return;
                final String pageUrl = j.optString("apkUrl", "");
                if (pageUrl.length() == 0) return;
                final String vname = j.optString("versionName", "");
                final String notes = j.optString("notes", "");
                act.runOnUiThread(() -> prompt(act, pageUrl, vname, notes));
            } catch (Exception e) {
                // offline / not deployed yet / bad manifest: silence
            }
        }).start();
    }

    private static int installedCode(Activity act) {
        try {
            PackageInfo pi = act.getPackageManager().getPackageInfo(act.getPackageName(), 0);
            if (Build.VERSION.SDK_INT >= 28) return (int) pi.getLongVersionCode();
            return pi.versionCode;
        } catch (Exception e) {
            return 0;
        }
    }

    private static String readAll(InputStream in) throws Exception {
        StringBuilder sb = new StringBuilder();
        byte[] buf = new byte[4096];
        int n;
        while ((n = in.read(buf)) > 0) sb.append(new String(buf, 0, n, "UTF-8"));
        return sb.toString();
    }

    private static void prompt(final Activity act, final String pageUrl, String vname, String notes) {
        if (act.isFinishing()) return;
        String msg = "Version " + vname + " of THE SURVIVAL GAME is ready.";
        if (notes != null && notes.length() > 0) msg += "\n\n" + notes;
        msg += "\n\nTap GET IT and the game's page opens in your browser — download the new build there and it installs as an update. Your saves carry over.";
        new AlertDialog.Builder(act)
                .setTitle("A NEW BUILD IS OUT")
                .setMessage(msg)
                .setPositiveButton("GET IT", (d, w) -> {
                    try {
                        Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(pageUrl));
                        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        act.startActivity(i);
                    } catch (Exception e) { /* no browser: nothing more we can do */ }
                })
                .setNegativeButton("NOT NOW", null)
                .show();
    }
}
