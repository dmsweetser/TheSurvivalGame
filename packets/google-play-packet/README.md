# Google Play packet — The Survival Game

Assembled 2026-11-07 (background preparation after Daniel chose
Google Play and the account check came back). This replaces nothing
— the Amazon and Game Jolt packets were trashed at Daniel's order
the same day, and the Steam kit is a separate, untouched decision.

## Contents

- `LISTING.md` — paste-ready Console copy (counts verified).
  Phone screenshots DONE: seven 1920×1080 captures in `assets/`
  (title, combat with a big hat, Bug Mall, stacked wall pile,
  Banzai World, Story Mode, Neon RAM Flats) — six from the Nov 9
  "Far Country" build, the Bug Mall shot recaptured Nov 10 from
  v1.3 — each visually verified — see LISTING.md.
- `FORMS.md` — Data Safety / ads / IARC answers pre-filled from the
  verified build; target audience and pricing flagged as Daniel's
  open decisions.
- `CLOSED-TEST-PLAN.md` — the full sequence now that the old
  "Morgan Road" account is confirmed closed (Sep 13, 2026):
  fresh account → upload keystore → Back-button/edge-to-edge fixes
  → closed test (12 testers × 14 days, clock starts at the 12th
  opt-in) → production application.
- `TESTERS.md` — blank tracker for the 12 + buffer testers.
- `PRIVACY-POLICY.md` — complete policy text; needs a public URL
  at submission time.
- `assets/icon-512.png`, `assets/feature-graphic-1024x500.png` —
  generated and visually checked Nov 7.

## Already done elsewhere

- AAB builds and validates: `../TheSurvivalGame.aab` (1.21 MB,
  API 36), itch/APK flow untouched.
- AAB REBUILT 2026-11-10 from the v1.3 build ("THE ARCADE
  OPENS"): `../TheSurvivalGame.aab` is now 1,205,549 bytes,
  bundletool-validated, and its embedded `base/assets/index.html`
  is byte-identical (md5 e3c8e43d103d571bb158402c9361ce88) to the
  current shipped game HTML. Still debug-key signed — a dedicated
  upload keystore is still needed at submission, and the
  pre-launch Back-button / edge-to-edge fixes in
  CLOSED-TEST-PLAN.md still stand. LISTING.md copy was revised
  2026-11-10 for the Arcade move, and the Bug Mall screenshot was
  recaptured the same day from v1.3 (Arcade entrance visible,
  visually verified) — the packet now has no stale assets.
- AAB REBUILT AGAIN 2026-11-12 from the v1.4 build ("FOLLOW THE
  YELLOW ARROW" — Tank Rescue mission chain, wider Arcade
  cabinets): `../TheSurvivalGame.aab` is now 1,216,697 bytes,
  bundletool-validated, versionCode 5 / versionName "1.4",
  targetSdk 36, and its embedded `base/assets/index.html` is
  byte-identical (md5 bc1f6cb14047155e0a9ee245a5262d13) to the
  v1.4 game HTML. Still debug-key signed — the dedicated upload
  keystore is still needed at submission. Screenshots were NOT
  recaptured: none of the seven shots depicts Tank Rescue or the
  Arcade interior, so v1.4 does not stale them; if a Tank Rescue
  shot is wanted for the listing, it needs a new capture.

## Daniel-only, in order

1. Free vs paid answer (recommended: free for release one).
2. New developer account: $25 + ID verification + Android device
   verification.
3. Recruit 14–15 testers.
