# THE SURVIVAL GAME — Complete Handover

*Everything you need to build, test, and publish the game yourself.
Prepared Nov 12, 2026. Current live version: **1.3** on itch.io.
**1.4 is built, tested, and staged in this package but not yet published**
(see §9 — a human-verification check on itch/Cloudflare is the only blocker).*

"SQUASH BUGS. WEAR THE EVIDENCE."

---

## 0. What's in this package

```
HANDOVER.md                  ← this file (the master guide)
BUILDING.md                  ← the repo's own build notes (short version)
project/                     ← the full source tree (was ~/workspace/survival-game)
releases/
  v1.4-ready/                ← the finished 1.4 builds + the itch browser zip + DEVLOG.md
  live-v1.3-share/           ← the "send to friends" pack matching what's live on itch now
packets/
  google-play-packet/        ← listing copy, forms, privacy policy, closed-test plan, assets
  steam-publish-kit/         ← store page draft, checklist, assets, steamcmd scripts
  whales-only-stripe.json    ← the 7 Whales Only Stripe product/payment-link IDs (not secret)
```

**Deliberately NOT in this package:** the Whales Only Ed25519 *private*
signing key (`whales-only-keys.json`). It lives only in the Cloudflare
Worker's secret store (and a master copy Muse holds). If you ever need
the raw value, ask Muse — it should travel by a secure channel, never
inside a zip. The Stripe *secret* key likewise lives only in the Worker.

---

## 1. How the game is put together

- **One C99 file does everything:** `project/src/survival.c` (~1 MB of C).
  Platform differences are `#ifdef`s: `PLAT_WASM`, `PLAT_WIN32`,
  `PLAT_LINUX`, `PLAT_MAC`. There is no engine and no dependency beyond
  libc-ish basics — the wasm build is freestanding; fonts and music are
  generated into the source by the scripts in `project/tools/`.
- **Web build:** `survival.c` compiles to wasm32-freestanding, gets
  gzip+base64'd, and is injected into `project/src/shell.html`, producing
  ONE self-contained HTML file. That single file is the product:
  - it is the itch browser game,
  - it is embedded into the native builds (`src/webbuild.h`, generated —
    **never hand-edit it**) and served by their built-in web server so
    friends join co-op from a browser,
  - it is the asset inside the Android APK/AAB (WebView wrapper).
- **Native builds** are the co-op hosts: launching one opens the game AND
  an HTTP+WebSocket server on **port 8379**; 1–4 browser guests join,
  guests can't pause. The Mac native build has no gamepad support yet.
- **Android** is a hand-rolled aapt2/javac/d8 pipeline (no Gradle) with
  two flavors generated from one manifest by `android/flavor_manifest.py`:
  - `itch` flavor (`src-itch/`): has INTERNET for the update *notice*
    (checks the version Worker, opens the itch page — it never downloads
    or installs APKs itself; the v1.1 self-installer got flagged
    "harmful app" by Play Protect, so 1.2+ is notice-only).
  - `play` flavor (`src-play/`): **zero permissions, no Stripe/payment
    links, collects and shares no data.**
- **Saves:** browser localStorage (web/APK) or native profile storage.
  Format is versioned (TSG3; TSG1/TSG2 still load). **Loading a save
  always resumes at the centre of the Bug Mall** — that's a rule, not a
  bug. Three save slots; local 2P co-op shares one slot.
- **Version of record:** `project/android/VERSION` (`CODE=5`, `NAME=1.4`).
  Both Android flavors read it; bump it BEFORE building a release.

## 2. Toolchain (what to install on your own machine)

| Tool | Version | Used for | Where to get it |
|---|---|---|---|
| Python | 3.10+ | `build.py`, asset scripts | python.org |
| **zig** | **0.14.1** | cross-compiler: wasm, Windows exe, macOS binary, `zig rc` for the exe icon/version | ziglang.org/download |
| System C compiler | gcc/clang | Linux native build | your distro |
| X11 + ALSA dev files | — | Linux build links `-lX11 -lasound` | `libx11-dev libasound2-dev` (Debian/Ubuntu) |
| JDK | 17 | Android builds | adoptium.net |
| Android SDK | build-tools **34.0.0**, platforms **android-34** (APK) and **android-36** (AAB) | APK + AAB | Google command-line tools |
| bundletool | 1.18.3 jar | AAB validation/build | github.com/google/bundletool/releases |
| Node.js | 18+ | Playwright test suites, `server.js` relay | nodejs.org |
| wrangler (optional) | latest | deploying the two Cloudflare Workers from CLI | `npm i -g wrangler` |

No Windows or Mac machine is needed — zig cross-compiles both from Linux.
(The Mac app is ad-hoc signed only: first launch needs right-click → Open.
Real signing/notarization needs an Apple Developer account — undecided,
see §8.)

**Path warning:** the scripts were written for this workspace layout and
contain absolute paths you must adapt on a new machine:
- `build.py` writes outputs to `~/workspace/your_files/` (`OUT`) and finds
  zig at `~/workspace/tools/zig-x86_64-linux-0.14.1/zig` (override with the
  `TSG_ZIG` env var).
- `android/build_apk.sh` reads the game HTML from
  `~/workspace/goals/the-survival-game/files/the-survival-game.html`,
  writes the APK next to it, and defaults `JAVA_HOME`/`ANDROID_HOME` to
  `~/workspace/tools/jdk17` and `~/workspace/tools/android-sdk`
  (both overridable by env var). `build_aab.sh` is the same shape.

## 3. Building

```
cd project
python3 build.py
```

One run produces everything (into `~/workspace/your_files/`):

| Output | What it is |
|---|---|
| `the-survival-game.html` | The complete web game (single file, ~1.45 MB) |
| `TheSurvivalGame.exe` | Windows, with icon + version info from `src/winres/survival.rc` |
| `TheSurvivalGame-linux` | Linux (X11 + ALSA) |
| `TheSurvivalGame-mac.zip` | macOS .app bundle (Apple Silicon), zipped |
| `TheSurvivalGame.apk` | Android APK — built by `android/build_apk.sh`, which build.py calls automatically when the SDK is present (also works on Fire TV) |
| `survival-game-source/` | small source bundle copied alongside |

Individual targets:
- Web only = the first step of build.py; there's no separate script —
  comment out the later steps or just let it build everything (it's fast).
- APK only: `bash android/build_apk.sh` (needs the HTML built first).
- AAB (Google Play) only: `bash android/build_aab.sh` — targets API 36,
  validates with bundletool, signs with `android/debug.keystore`
  (debug key — fine for testing; a real submission needs a proper upload
  key, see §7).
- Windows from Windows: `build.bat` (wasm + exe, needs zig on PATH).

**Smoke test after any build:** run the Linux binary headlessly
(`xvfb-run ./TheSurvivalGame-linux`) — the native builds embed the web
build in a static buffer, and the Linux build is the canary that aborts
loudly if that buffer ever overflows. Also eyeball the printed byte
sizes against the last known-good set (§9).

## 4. Tests

Playwright suites live in `project/tests/` (~135 scripts; suites end in
`test.js`, probes/shots are diagnostics). They drive the web build in
headless Chromium through the game's `dbg_*` hooks.

```
cd project/tests && npm install            # playwright-core (once)
python3 -m http.server 8903 --directory ~/workspace/your_files   # serve the build
node tanktest.js                           # Tank Rescue mission chain
node arcadetest.js                         # arcade cabinets
node entrancetest.js                       # Entrances mode
# …etc.
```

Hard-won rules (each cost hours once):
- **Redirect every suite to its own log file and read the file**
  (`node tanktest.js > tank.log 2>&1`). Never trust a piped `| tail`.
- Hold keys ~150 ms — instant presses fall between wasm frames and vanish.
- Navigate menus verify-and-retry against `dbg_*` state; never assert on
  a fixed-duration walk.
- Regression saves must use realistic play state (hat ON, money banked,
  modes visited) — clean-state saves once hid a save-parser bug that ate
  the buddy/bank/progress from real saves.
- Never launch a suite while a build is mid-write; run suites one at a
  time (the machine's memory won't take parallel Chromium).
- The key suites for a release: tanktest, arcadetest, bwtest, gatetest,
  entrancetest, crafttest, platformtest, banktest, tilltest, hatsavetest,
  beanjettest, kbmobiletest, buddycontinue.

## 5. Release checklist (every version)

1. Bump `project/android/VERSION` (CODE +1, NAME) **before** building.
2. `python3 build.py` (+ `build_aab.sh` if the Play packet needs refreshing).
3. Run the key test suites (§4); read the logs.
4. Smoke-test the Linux binary under Xvfb.
5. Stage the itch uploads and **verify byte sizes/md5 against the fresh
   builds** — the goal-folder copies of the exe/linux have silently been
   stale before; stage with `cp -L` (some entries are symlinks).
   Browser zip: the HTML as `index.html` at the zip root (a ready one is
   in `releases/v1.4-ready/`).
6. Publish to itch (§6) **with a devlog in the same trip** (§6.3 — your
   standing rule).
7. Bump the update-server Worker to the new version (§6.4) and verify
   the live endpoint actually serves the new version before saying done.
8. Refresh the Google Play packet AAB if a Play push is planned (§7).

## 6. Deploy: itch.io (the live channel)

Project: **https://dmsweetser.itch.io/the-survival-game** (account `dmsweetser`).

### 6.1 Files to upload (six)
- `the-survival-game.html` — uploaded TWICE in the project file list: one
  copy flagged "played in the browser" (this is the browser build — the
  single-HTML format replaced the old browser zip in the v1.3 publish),
  and one copy as the standalone single-file download (not browser-played)
- `TheSurvivalGame.exe`, `TheSurvivalGame-linux`, `TheSurvivalGame-mac.zip`
- `TheSurvivalGame.apk`

### 6.2 Browser-play settings (check EVERY publish)
Replacing the browser build **clears its "played in browser" flag**.
After uploading, re-confirm on the project edit page: the browser file
is marked playable in browser, embed size **1280×720**, auto-start on,
fullscreen button on. If the flag is off, the game won't launch on the
page even though everything uploaded fine.

### 6.3 Devlog — with EVERY publish (your rule, Nov 3)
Write it in the game's voice, end with **SQUASH BUGS. WEAR THE
EVIDENCE.** The 1.4 devlog ("FOLLOW THE YELLOW ARROW") is ready in
`releases/v1.4-ready/DEVLOG.md`.

### 6.4 Android update server (Cloudflare Worker `tsg-version`)
- Source: `project/android/update-server/` (`worker.js`, `wrangler.toml`,
  README with the dashboard route).
- Live endpoint the APK checks:
  `https://tsg-version.dmsweetser.workers.dev/version.json`
- After each itch release: edit `LATEST` in `worker.js` (versionCode,
  versionName, apkUrl = the APK's file URL on the itch project page,
  one-line notes), then redeploy (dashboard paste or `wrangler deploy`).
- **Verify** with `curl https://tsg-version.dmsweetser.workers.dev/version.json`
  — as of this handover it still serves **1.3**; the 1.4 edit exists
  locally in `worker.js` but was never deployed (see §9).

### 6.5 Whales Only (Stripe + issuer Worker)
- In-game counter in the Bug Mall (old Gallery slot). Seven cosmetic
  products, all **$0.67**, created on your live Stripe account — IDs and
  payment links in `packets/whales-only-stripe.json`.
- Buyer flow: game → Stripe Payment Link → Stripe redirects to the
  issuer Worker's `/done` page → Worker verifies the checkout session
  with Stripe and shows a **signed unlock code** → the game verifies the
  code **offline** (Ed25519 public key is compiled into the game).
- Issuer Worker source: `project/whales-only/` (`worker.js`,
  `wrangler.toml`, README with full deployment steps). Worker name:
  `whales-only-issuer`. Its two secrets (Stripe restricted key with
  Checkout-Sessions-read only, and the Ed25519 private JWK) live in the
  Worker's secret store — see §0 for what's deliberately not in this zip.
- Android builds contain **no** Stripe links (buying is web/native only).

## 7. Deploy: Google Play (chosen channel — not yet submitted)

Packet: `packets/google-play-packet/` — README (master checklist),
LISTING.md, FORMS.md (console form answers), PRIVACY-POLICY.md,
CLOSED-TEST-PLAN.md, TESTERS.md, assets/, and the AAB is at
`releases/v1.4-ready/TheSurvivalGame.aab` (v1.4, API 36,
bundletool-validated).

Still needed, and **only you can do these**:
1. Create a fresh developer account — **$25** one-time + ID verification.
   (Your old "Morgan Road" account was closed by Google Sep 13, 2026 for
   inactivity.) Free-vs-paid for the game itself is still your open call.
2. As a new personal account, production access requires a **closed test
   with 12+ testers for 14 continuous days** first (plan in the packet).
3. Create a dedicated **upload keystore** and enroll in Play App Signing —
   the AAB in this package is debug-key signed, which Play won't accept
   for a real submission.
4. Pre-launch code fixes still open: predictive Back on Android 16 and
   edge-to-edge layout — both need a code change + a real-device test.

## 8. Steam (kit ready, decisions open)

`packets/steam-publish-kit/`: CHECKLIST.md, STORE_PAGE.md (draft copy),
assets/, steamcmd upload scripts. `project/make_steam_assets.py`
regenerates the capsule art. Open decisions, all yours: price
($4.99 recommended vs free), publisher display name, individual vs
company, and a trailer still needs making. Signing/notarization for the
Mac build ties in here if Steam ever ships.

## 9. Current status (Nov 12, 2026)

- **Live:** v1.3 "THE ARCADE OPENS" on itch (devlog:
  dmsweetser.itch.io/the-survival-game/devlog/1767281/the-arcade-opens).
  Update Worker serves 1.3. `releases/live-v1.3-share/` matches it.
- **v1.4 is DONE and in this package** (`releases/v1.4-ready/`): Tank
  Rescue reworked toward NES Jackal (yellow-arrow mission chain:
  rescue → deliver 3 → break the Blockade Gun → destroy the Fortress
  Core → finish line; tougher enemies; shorter player cannon range),
  arcade cabinets widened 108→128px so SUPER BUDDY RUN fits its
  nameplate, and (added Nov 14) the **WALK OF FAME**: crawling through
  the Bug Mall under a heavy hat (speed < 64 px/s) rains confetti and
  flashes a WALK OF FAME marquee — see `tests/woftest.js` (12/12).
  Also fixed Nov 14: the title-menu layout now LATCHES its pad/keyboard
  flavour while the menu is open (`menu_pad_mode()` in survival.c).
  Before, pad_mode() followed the last device touched, so one keyboard
  press re-inserted the SETTINGS row mid-navigation and pushed the
  2P CO-OP row down one — co-op could never be toggled by row-counting,
  which is what broke local co-op drop-in (mallfix page 2).
  Suites: tanktest 20/20, arcadetest 24/24, woftest 12/12, hatsave 6/6,
  mallfix 28/28. Checksums (Nov 14 rebuild, post-fix):
  - html md5 `ffe6a211aaa69c078cdd1ab5f5bd3340` (1,456,327 B)
  - exe md5 `ee31a1c9776637dd822b4a02033b5457` (2,635,776 B)
  - linux md5 `602495851ab126269403e074a98c03fe` (3,504,904 B)
  - mac.zip md5 `8e975ba0ca5ed2f4ffd45cf7fd1dd64b` (1,597,606 B)
  - apk md5 `80a0cd87925c61ef7f559b739eacc063` (1,225,964 B), versionCode 5 / "1.4"
  - aab md5 `f2c9d472b2565529e9c7cfbf8b582f0e` (1,218,182 B)
- **Why 1.4 isn't live:** itch's login and the Cloudflare dashboard both
  sit behind a Cloudflare "Verify you are human" check that loops for
  Muse's automated browser (tried Nov 11–12, many routes). To publish:
  1. Sign in to itch.io as `dmsweetser` in your own browser.
  2. Upload the six files from `releases/v1.4-ready/` (browser zip first).
  3. Re-check the browser-play settings (§6.2).
  4. Post the devlog from `releases/v1.4-ready/DEVLOG.md`.
  5. Then redeploy the `tsg-version` Worker with the 1.4 values already
     edited into `project/android/update-server/worker.js`, and curl the
     endpoint to confirm it serves 1.4.
  Or just tell Muse "take over my browser" — with you clearing the
  human check once, Muse can run the whole publish in that session.
- **Facebook Instant Games: parked** (your call, Nov 12). Going live
  needs Meta's Approved Partner Program (verified business entity +
  application; 180-day wait if rejected), IAP minimum $0.99 (the $0.67
  Whales Only price can't exist there), 70/30 split. A Development-Mode
  build remains possible if you ever revive it.
- **Amazon / Game Jolt:** packets were deleted Nov 7 (your call) — not
  part of this handover.

## 10. Accounts & where things live

| Thing | Where |
|---|---|
| itch.io project | dmsweetser.itch.io/the-survival-game (project 5258640) |
| Stripe | your live account; 7 Whales Only products (IDs in this package) |
| Cloudflare | account `dmsweetser` — Workers `tsg-version` + `whales-only-issuer` |
| Google Play | no active account (see §7) |
| Game source of truth | `project/` in this package |
| Player-facing help | `releases/live-v1.3-share/PLAY-WITH-FRIENDS.md` (Fire TV / Kindle Fire paths included) |

## 11. Invariants — don't break these

- "Wheedle" appears **nowhere** — the puffball critter is **Eddie**, your
  own original character. Never market him as anything else.
- Public copy dedicates to "my wife" — never family members' names.
- Devlog with **every** itch publish, game voice, tagline at the end.
- Save/load resumes at the Bug Mall centre, always.
- The Play-flavor Android build keeps zero permissions and no payment links.
- Native builds stay — they're the co-op hosts and have the sturdier saves.
- If a deploy can't be verified at the live endpoint/page, it isn't done —
  check, don't assume.
