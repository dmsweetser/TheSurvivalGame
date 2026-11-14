# tsg-version — update manifest Worker

A tiny Cloudflare Worker for **The Survival Game**'s itch/direct-download
Android build. It serves one JSON file (`/version.json`) that the APK's
launch-time update check reads. Sibling of `whales-only-issuer`, deployed
the same way.

## Deploy (dashboard route, ~2 minutes)

1. Cloudflare dashboard → **Workers & Pages → Create → Worker**.
2. Name it exactly `tsg-version` (the APK hardcodes
   `https://tsg-version.dmsweetser.workers.dev/version.json`).
3. Replace the placeholder code with `worker.js` from this folder and deploy.

Until this is deployed, the update check fails silently and the game
plays exactly as before — no rush, no breakage.

## Publishing checklist (every itch release, after the upload)

1. Bump `android/VERSION` (CODE +1, NAME as appropriate) **before**
   building, so the new APK carries the new identity.
2. Upload the builds to itch as usual.
3. Edit `LATEST` in `worker.js`: `versionCode` = the new CODE,
   `versionName` = the new NAME, `apkUrl` = the direct download URL
   of `TheSurvivalGame.apk` from the itch project page, `notes` = one
   short line.
4. Redeploy the Worker (paste the updated `worker.js`).

Players on older builds then get "A NEW BUILD IS OUT" at next launch:
one tap downloads, one system confirm installs, saves carry over.

## What it costs

- Free tier is plenty (one static JSON).
- The Worker's logs see standard request metadata (IP, user agent) —
  the app itself sends nothing and collects nothing. This service is
  only used by the itch flavor; the Play build never contacts it.
