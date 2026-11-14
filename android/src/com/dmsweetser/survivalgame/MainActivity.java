package com.dmsweetser.survivalgame;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Base64;
import android.view.View;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;

import java.io.File;
import java.io.FileOutputStream;

// The Survival Game — Android wrapper. The whole game is the single-file
// WebAssembly build (the-survival-game.html) bundled as an asset; this
// activity is just a fullscreen WebView to run it in. Single player here:
// LAN hosting lives in the desktop builds, whose built-in web server is
// how phones join as guests in a browser.
public class MainActivity extends Activity {
    private WebView web;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        web = new WebView(this);
        web.setBackgroundColor(0xFF14121F);   // the game's own dark: no white flash while it loads
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);            // the game saves to localStorage
        s.setMediaPlaybackRequiresUserGesture(false);
        web.addJavascriptInterface(new ShareBridge(), "AndroidShare");
        web.setWebViewClient(new android.webkit.WebViewClient());
        setContentView(web);
        enterImmersive();
        // #apk tells the game's shell to use app presentation: mobile UI
        // plus a small margin around the play area.
        web.loadUrl("file:///android_asset/index.html#apk");
        // Flavor hook: the itch build checks for a newer build here;
        // the Play build's Updater is a no-op stub (Play updates it).
        Updater.maybeCheck(this);
    }

    private void enterImmersive() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) enterImmersive();
    }

    // Back acts as the game's Esc (pause / leave store / cancel), the way
    // the desktop builds treat it. Home/recents still leave the app.
    @Override
    public void onBackPressed() {
        web.evaluateJavascript(
                "window.dispatchEvent(new KeyboardEvent('keydown',{code:'Escape',key:'Escape'}));" +
                "window.dispatchEvent(new KeyboardEvent('keyup',{code:'Escape',key:'Escape'}));",
                null);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (web != null) web.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (web != null) web.onResume();
    }

    // Share bridge: the game-over screen's SHARE button hands us the
    // score text plus the score-card photo (base64 PNG); we turn it into
    // a normal Android share intent (photo attached, text with the link).
    public class ShareBridge {
        @JavascriptInterface
        public void share(final String text, final String imageB64) {
            runOnUiThread(() -> {
                try {
                    byte[] png = Base64.decode(imageB64, Base64.DEFAULT);
                    File f = new File(getCacheDir(), "share.png");
                    FileOutputStream out = new FileOutputStream(f);
                    out.write(png);
                    out.close();
                    Uri uri = Uri.parse("content://com.dmsweetser.survivalgame.share/share.png");
                    Intent send = new Intent(Intent.ACTION_SEND);
                    send.setType("image/png");
                    send.putExtra(Intent.EXTRA_STREAM, uri);
                    send.putExtra(Intent.EXTRA_TEXT, text);
                    send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    startActivity(Intent.createChooser(send, "Share your score"));
                } catch (Exception e) {
                    // sharing is best-effort; never crash the game over it
                }
            });
        }
    }
}
