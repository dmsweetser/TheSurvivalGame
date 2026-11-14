// tsg-version — the update manifest for The Survival Game's
// itch/direct-download Android build. The APK's launch-time check
// fetches /version.json from this Worker; if versionCode here beats
// the installed build, the game offers a one-tap update.
//
// TO PUBLISH A NEW BUILD: upload the APK to itch as usual, then edit
// LATEST below (bump versionCode, set versionName + the direct APK
// download URL from the itch project page, short notes) and redeploy
// this Worker (paste into the Cloudflare dashboard, like the
// whales-only-issuer deployment). The game-side version identity
// lives in android/VERSION — the two must agree.
//
// The Play build never talks to this Worker (Play updates it).

const LATEST = {
  versionCode: 5,
  versionName: "1.4",
  // Direct download URL of TheSurvivalGame.apk on itch — update this
  // if the file URL changes after an upload.
  apkUrl: "https://dmsweetser.itch.io/the-survival-game",
  notes: "TANK RESCUE GETS ITS ORDERS: follow the yellow arrow - rescue the captives, deliver three to the pads, break the Blockade Gun, then destroy the Fortress Core. Enemies hit harder now, and the arcade cabinets got wider.",
};

export default {
  async fetch(request) {
    const url = new URL(request.url);
    if (url.pathname === "/" || url.pathname === "/version.json") {
      return new Response(JSON.stringify(LATEST), {
        headers: {
          "Content-Type": "application/json",
          "Cache-Control": "public, max-age=3600",
        },
      });
    }
    return new Response("The Survival Game — version service. The game checks /version.json here.", {
      headers: { "Content-Type": "text/plain" },
    });
  },
};
