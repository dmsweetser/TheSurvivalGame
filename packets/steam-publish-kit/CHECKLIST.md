# Publishing THE SURVIVAL GAME to Steam — the whole path

Short version: Steam publishing = a Steamworks partner account + a
$100 per-game fee (recouped at $1,000 gross) + a store page + a build
upload + Valve's review. Everything creative is already prepared in
this kit; the identity/money steps can only be done by you.

## Phase 0 — Decisions (5 minutes)

- [ ] **Know this first:** Steam has *no* free draft mode — the $100
      fee must be paid before an App ID exists, so nothing can be
      shared through Steam before paying. (After paying, an unreleased
      app + beta keys works fine for private friend testing.) For free
      sharing right now, use the `survival-game-share` folder: send
      the .exe, or put the browser build on itch.io as a free
      draft/restricted page — see `PLAY-WITH-FRIENDS.md` there.

- [ ] **Price:** $4.99 recommended (math in STORE_PAGE.md) — or free.
      One-way door either way, decide once.
- [ ] **Publisher name** shown on the page: your name, or a studio
      name. (Can match how you published "Hope Starts Small".)
- [ ] **Individual vs company:** individual is the simple path; the
      name on the tax form must match the name on the bank account.
- [ ] **Smoke-test the .exe first.** `steamcmd/content/TheSurvivalGame.exe`
      has been cross-compiled but never run on a real Windows machine —
      double-click it on yours before uploading anything. (Steam's
      review will run it too.)

## Phase 1 — Your account steps (~30–45 min, only you can do these)

At https://partner.steamgames.com (sign in with your Steam account;
turn on Steam Guard if it isn't already):

- [ ] Sign the Steamworks Distribution Agreement.
- [ ] Verify your identity.
- [ ] Add banking details (Valve pays monthly by ACH; bank account
      name must match the tax-form name).
- [ ] Complete the **tax interview** — as a US individual this is a
      W-9.
- [ ] **Pay the $100 Steam Direct fee** for this app (credit/debit
      card — Steam Wallet funds are NOT accepted). Non-refundable, but
      recouped once the game passes $1,000 adjusted gross revenue.
- [ ] You now have an **App ID**. Note it (and the depot ID, usually
      App ID + 1) — the upload scripts need them.

Timing rules to know: the fee must be paid **at least 30 days before
your release date**, and your store page must be public ("Coming
Soon") **at least 2 weeks** before release. So the fastest honest
launch is ~5–6 weeks after you start Phase 1.

## Phase 2 — Store page (all content prepared in this kit)

In Steamworks → your app → Store Page:

- [ ] Name: **The Survival Game**
- [ ] Short + long description: paste from `STORE_PAGE.md`
- [ ] Pricing + feature checkboxes + system requirements + tags:
      also in `STORE_PAGE.md`
- [ ] Upload the images from `assets/` (filenames say the size):
      4 capsules, 6 screenshots (1920×1080), library capsule/hero/
      logo, community icon 184×184, shortcut icon 256×256, optional
      page background. The library hero correctly has **no text**.
- [ ] Complete the **IARC age-rating questionnaire** (free, ~10 min;
      answer notes in `STORE_PAGE.md`) and the content/AI disclosures.
- [ ] **Trailer — the one asset not in this kit.** Steam expects a
      gameplay trailer. Best route: on your Windows machine, run the
      .exe, record 30–45 s of play with Xbox Game Bar (Win+G) — real
      speed *and* the real MIDI music — and upload the MP4 (H.264).
      Trim the menu out; gameplay in the first 10 seconds.
- [ ] Set the page to **Coming Soon** and submit it for review.

## Phase 3 — Upload the build (15 min, your machine)

- [ ] Download the **Steamworks SDK** (Steamworks → Tools) and put
      this kit's `steamcmd/` folder alongside `steamcmd.exe`
      (the SDK's `tools/ContentBuilder/` folder is exactly this shape).
- [ ] Fill your App ID / depot ID into the two `.vdf` templates.
- [ ] Run `upload_build.bat`; log in with your Steam credentials +
      Steam Guard code when steamcmd asks.
- [ ] In Steamworks set the launch option: executable
      `TheSurvivalGame.exe`, install dir = the depot root.
- [ ] Optional but nice: **Steam Auto-Cloud** — point it at
      `TheSurvivalGame_save.txt` in the install folder so saves follow
      players between PCs. No code change needed.
- [ ] Submit the build for review. (Store page and build reviews run
      ~2–5 business days each and can bounce back with fix requests.)

Note on multiplayer: LAN co-op works as-is over UDP (host enters/
shares their local IP). There's no Steam matchmaking or online
servers — the store copy says LAN co-op, so expectations match.

## Phase 4 — Release

- [ ] Pick a release date ≥30 days after the fee and ≥2 weeks after
      Coming Soon went public.
- [ ] When both reviews pass, press Release in Steamworks.
- [ ] Watch the first reviews; the heaviest-hat jokes write themselves.

## Costs at a glance

- $100 one-time fee (recouped at $1,000 gross ≈ 200 copies at $4.99)
- Steam keeps 30% of revenue; you keep 70%
- Nothing else: no listing fees, no per-update fees.

## What's in this kit

- `STORE_PAGE.md` — all store copy, features, requirements, tags,
  pricing math, rating-questionnaire notes
- `assets/` — every required image, generated from real gameplay
  captures; title set in a Comic Sans style (Comic Neue Bold, the
  open-license Comic Sans revival) per your direction
- `steamcmd/` — the game .exe in `content/`, SteamPipe build scripts
  with ID placeholders, and a double-click upload .bat
- (Rebuild assets any time with `~/workspace/survival-game/make_steam_assets.py`.)
