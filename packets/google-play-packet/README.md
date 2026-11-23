# Google Play packet — The Survival Game

Assembled 2026-11-07 (background preparation after Daniel chose
Google Play and the account check came back). This replaces nothing
— the Amazon and Game Jolt packets were trashed at Daniel's order
the same day, and the Steam kit is a separate, untouched decision.

## Contents

- `LISTING.md` — paste-ready Console copy (counts verified).
  Phone screenshots DONE: eight 1920×1080 captures in `assets/`
  (title, combat with a big hat, Bug Mall, stacked wall pile,
  Banzai World, Story Mode, Neon RAM Flats, Mordor Fun Run) — six from the Nov 9
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
  API 36), GitHub/APK flow untouched. (itch was retired
  2026-11-14 at Daniel's direction; GitHub is the game's home.)
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
- AAB REBUILT AGAIN 2026-11-14 from the v1.6 build ("START YOUR
  ENGINES" — sixth Arcade cabinet SUPER BUDDY CART on a new second
  row, over the v1.5 content): `../TheSurvivalGame.aab`
  is now 1,274,661 bytes, versionCode 7 / versionName "1.6",
  targetSdk 36, and its embedded `base/assets/index.html` is
  byte-identical (md5 c26cdc86f2069fefaa0965e8434ef7e7) to the
  v1.6 game HTML (verified from the AAB archive this run;
  bundletool validation was reported for the v1.5 release build in
  the main chat, not re-run here). Still debug-key signed.
  LISTING.md copy revised the same day for the kart cabinet.
  `assets/icon-512.png` REGENERATED 2026-11-14 from the current
  icon master (the thicker-frown spider), so the listing icon now
  matches the shipped app icon. Screenshots: the title shot was
  RECAPTURED 2026-11-14 from the v1.5 build via
  `tests/playtitleshot.js` (1920×1080, md5
  e7809d38f182f704aa138e039b8c70ee, visually verified — trimmed
  header band and clear records strip), so the packet again has
  no stale assets; the other six scenes are not depicted as
  changed by v1.5 (CASTLE lives inside the Arcade, which no shot
  depicts).
- AAB REBUILT AGAIN 2026-11-14 from the v1.7 build ("MODE 7" —
  SUPER BUDDY CART rebuilt SNES-style from behind the kart with
  the Banzai Buddies themselves driving, the circuit dressed,
  drift-with-mini-boost physics, and TANK RESCUE gentler with
  switchable grenades): `../TheSurvivalGame.aab` is now
  1,311,948 bytes, versionCode 8 / versionName "1.7",
  targetSdk 36, and its embedded `base/assets/index.html` is
  byte-identical (md5 a0b9130a8b3bc697822727cf881a5bdc) to the
  current v1.7 game HTML (verified from the AAB manifest and
  archive 2026-11-14; bundletool validate run the same day).
  Still debug-key signed — the dedicated upload keystore is
  still needed at submission. LISTING.md copy revised the same
  day for the Mode 7 kart and the Tank Rescue grenades.
  Screenshots were NOT recaptured: no shot depicts the kart
  race or Tank Rescue, and the title and mall scenes are
  unchanged since their v1.5/v1.3 captures, so the packet
  again has no stale assets.
- AAB RESTAMPED 2026-11-14 to v1.9 ("THE BUDDIES TAKE THE
  WHEEL" — at Daniel's direction the versions were aligned at
  versionCode 9 / versionName "1.9"; v1.8 and v1.9 are the
  v1.7 game build under its final number, with no game-code
  change): `../TheSurvivalGame.aab` is now 1,311,947 bytes,
  versionCode 9 / versionName "1.9", byte-identical (md5
  d81a05a1907a354cca6f9541c662470a) to the GitHub v1.9
  release AAB, and its embedded `base/assets/index.html` is
  byte-identical (md5 a0b9130a8b3bc697822727cf881a5bdc) to
  the v1.9 game HTML (both verified from the files
  2026-11-14; the sibling release APK's aapt badging reads
  versionCode 9 / versionName "1.9"). Still debug-key signed
  — the dedicated upload keystore is still needed at
  submission. LISTING.md copy is unchanged: it already
  describes this exact game build. Screenshots were NOT
  recaptured: no scene changed in the renumber.
- AAB RESTAMPED 2026-11-14 to v1.10 ("FIRE IS GO" — the
  SUPER BUDDY CART overhaul: hold FIRE to go with the stick
  steering only, plus a full track makeover; re-cut as its
  own version the same day under Daniel's new rule that a
  major change gets a new version): `../TheSurvivalGame.aab`
  is now 1,319,381 bytes, versionCode 10 / versionName
  "1.10", byte-identical (md5 50bce99edbd56ef97220efb627dc6fad)
  to the GitHub v1.10 release AAB, and its embedded
  `base/assets/index.html` matches the v1.10 game HTML
  (md5 8916b7098776380ec178fc3b7c460911; the sibling release
  APK's aapt badging verified versionCode 10 / versionName
  "1.10"). Still debug-key signed — the dedicated upload
  keystore is still needed at submission. LISTING.md copy
  is unchanged: it never described the kart control scheme.
  Screenshots were NOT recaptured: no shot depicts the kart
  race, and the title and mall scenes are unchanged.
- AAB RESTAMPED 2026-11-14 to v1.11 ("OUT FROM THE
  EPICENTER" — SUPER BUDDY RUN and SUPER BUDDY CART aligned
  to the main game's 8 regions: RUN levels wear their
  region's palette and bug strain and gain stomp chains and
  a bonus vault pipe; CART becomes a Grand Prix — 8 regional
  circuits, HOME and FAR cups of 4 races, points, standings,
  and a saved final rank per cup; BUDDY CRAFT gains a search
  field; CASTLE no longer lets you leave without the key):
  `../TheSurvivalGame.aab` is now 1,342,121 bytes,
  versionCode 11 / versionName "1.11", byte-identical
  (md5 d9a0979db54aca9518b3fa7ec3f73017) to the GitHub v1.11
  release AAB, and its embedded `base/assets/index.html`
  matches the v1.11 game HTML
  (md5 48e42587f1fe9dc0626e3658662bc8e7; the sibling release
  APK's aapt badging verified versionCode 11 / versionName
  "1.11"). Still debug-key signed — the dedicated upload
  keystore is still needed at submission. LISTING.md copy
  is unchanged: it never described level themes, the kart
  format, or the castle door. Screenshots were NOT
  recaptured: no shot depicts the Run, the kart race, or
  the castle, and the title and mall scenes are unchanged.
- AAB RESTAMPED 2026-11-15 to v1.12 ("STEADY FEET" — SUPER
  BUDDY RUN slowed ~25% at the top end with much stronger
  friction, and on the touch stick scheme the level rides
  on an 84px deck clear of the joystick and JUMP button;
  SUPER BUDDY CART's grandstand moved back from the grid,
  size-clamped, and culled at close range — it had been
  projecting as a screen-filling gray wall; main-game wall
  runs now share their health pool exactly — 1.1x the sum
  of segment bases, with reinforcement credited at full
  strength and no stale-health carries):
  `../TheSurvivalGame.aab` is now 1,345,300 bytes,
  versionCode 12 / versionName "1.12", byte-identical
  (md5 d92c6e3498780aa0224112335fe5d592) to the GitHub v1.12
  release AAB, and its embedded `base/assets/index.html`
  matches the v1.12 game HTML
  (md5 e565b51538412e3b867af9d4487b1e8a; the sibling release
  APK's aapt badging verified versionCode 12 / versionName
  "1.12"). Still debug-key signed — the dedicated upload
  keystore is still needed at submission. LISTING.md copy
  is unchanged: it never described the Run's speed or touch
  layout, the grandstand, or wall bookkeeping. Screenshots
  were NOT recaptured: no shot depicts the Run, the kart
  race, or a wall run, and the title and mall scenes are
  unchanged.
- AAB RESTAMPED 2026-11-15 to v1.13 ("WAR!" — a seventh
  Arcade cabinet: a real-time strategy mode. A world map
  rings the 8 regions around the epicenter; each battle is
  a 1600x1200 arena in the region's palette — harvesters
  haul BITS from code nodes to the COMPILER CORE, which
  (with the PATCH BAY) trains harvester, melee and ranged
  buddies; ZAPPER TOWERS and FIREWALLS are the defensive
  builds; 1-3 bug hives per region spawn guards and attack
  waves, and razing every hive conquers the region while
  losing the Core loses the battle. Tap to select,
  double-tap selects all of a type, tap to move/attack,
  drag to pan; desktop mouse clicks now route through the
  same tap channel game-wide. Conquest persists on the
  buddy under save key "wr". The Arcade room grew a third
  row (south wall and clamps moved ~170px); the other six
  cabinets did not move. Solo command in v1 — no shared
  co-op battle):
  `../TheSurvivalGame.aab` is now 1,412,460 bytes,
  versionCode 13 / versionName "1.13", byte-identical
  (md5 8bd62e0fde6bb2f3fd2c402a81965a89) to the GitHub v1.13
  release AAB, and its embedded `base/assets/index.html`
  matches the v1.13 game HTML
  (md5 8fe10dfe6bd611586f7a2b1f427113a5; the sibling release
  APK's aapt badging verified versionCode 13 / versionName
  "1.13"). Still debug-key signed — the dedicated upload
  keystore is still needed at submission. LISTING.md copy
  CHANGED this time: the Arcade enumeration gained WAR!
  (full description re-counted at 2,961 of 4,000 chars).
  Screenshots were NOT recaptured: no shot depicts the
  Arcade interior or a WAR! battle, and the title and mall
  scenes are unchanged.
- AAB RESTAMPED 2026-11-16 to v1.14 ("WAR MACHINES" — a
  WAR!-only update from Daniel's first playtest: building
  now requires the property zone radiated by your own
  structures, shown while placing, with firewall segments
  extending the line; triple-click selects all combat
  units (single = one, double = all of a type, harvesters
  excluded); deterministic terrain per region — trees,
  rocks, broken terminals, cable spools — blocking
  movement but not shots; firewall placement snaps to a
  24u grid and segments render connected into continuous
  barriers; a new ASSEMBLY PLANT (200 bits) builds SCOUT
  BUGGIES (90 bits, fast, sight 230) and SIEGE TANKS
  (160 bits, slow, armoured, x2 damage vs structures);
  permanent fog of war — explored ground never re-fogs,
  enemy units show only in current sight; and the battle
  drag-pan now grabs the world, finger-tracking on both
  touch schemes):
  `../TheSurvivalGame.aab` is now 1,431,523 bytes,
  versionCode 14 / versionName "1.14", byte-identical
  (md5 b8d878706e004f00e6cccae64f1d98a9) to the GitHub v1.14
  release AAB, and its embedded `base/assets/index.html`
  matches the v1.14 game HTML
  (md5 6ed0b681c1002329125545ee7a1790f4; the sibling release
  APK's aapt badging verified versionCode 14 / versionName
  "1.14"). Still debug-key signed — the dedicated upload
  keystore is still needed at submission. LISTING.md copy
  changed again: the WAR! sentence grew vehicles, property,
  connected firewalls and fog (full description re-counted
  at 3,198 of 4,000 chars). Screenshots were NOT
  recaptured: no shot depicts a WAR! battle, and the title
  and mall scenes are unchanged.

- AAB RESTAMPED 2026-11-20 to v1.25 ("FIRST SHIFT" — the
  combined 16-item feature release: First Shift, Comfort
  Settings, Medal Times, contextual HUD prompts, Trophy
  Case, Hat Toppers, Weekly Challenge, Castle Shuffle,
  Named Elites + Bounty Board, Hat Portrait Booth, Bug Bowl
  League, Entrances Chapter 2, day/night tint + regional
  weather, WAR! local two-commander co-op, the Banzai World
  post-game chapter, and adaptive music intensity — plus
  the enlarged Bug Mall and the threat-size stretch):
  `../TheSurvivalGame.aab` is now 1,728,510 bytes
  (md5 62f7da7c127e5b525ec69df75c12c5ac), and its embedded
  `base/assets/index.html` is byte-identical
  (md5 231b6c09dd70f0d007cb0d72abe0b6da) to the shipped
  v1.25 game HTML — the same md5 verified live on GitHub
  Pages at ship time. The sibling APK's aapt badging
  verifies versionCode 25 / versionName "1.25". This AAB
  is the precautionary-rebuild artifact of 2026-11-20,
  checksum-verified against the fresh builds after the
  release assets were replaced in place (same v1.25
  release, no new version). Still debug-key signed — the
  dedicated upload keystore is still needed at submission.
  ONE PACKET ITEM IS NOW STALE and flagged here rather
  than silently carried: `assets/screenshot-03-mall.png`
  (captured 2026-11-10 from v1.3) depicts the old
  568×322 Mall — v1.25 enlarged the interior to 768×402
  with nine stops, so that shot needs recapture. The
  other six screenshots depict scenes not changed by
  v1.20–v1.25. Nothing was submitted anywhere.
- LISTING REVISED 2026-11-20 to v1.25 (resolving the stale
  flag above): LISTING.md copy now describes the shipped
  v1.25 build — the 9-stop enlarged Mall with Trophy Case,
  Hat Portrait Booth and Bounty Board, CASTLE Shuffle,
  Entrances Chapter 2, WAR! local two-commander co-op, the
  Banzai World post-game chapter, First Shift, Medal Times,
  Weekly Challenge, Hat Toppers, day/night + regional
  weather, adaptive music and Comfort Settings — trimmed
  elsewhere to fit. Character counts re-verified this run:
  title 17/30, short description 64/80, full description
  3,990/4,000 counting newlines (3,926 excluding them) —
  the v1.19 text measured 4,037 counting newlines, over
  the limit under that counting, so the revision also
  brings the count safely under both ways. The mall
  screenshot flag stands: screenshot-03-mall.png still
  depicts the old 568×322 Mall and needs recapture before
  any submission. Nothing was submitted anywhere.
- SCREENSHOT RECAPTURED 2026-11-20 (resolving the flag
  above): `assets/screenshot-03-mall.png` was recaptured
  from the shipped v1.25 build via `tests/playmallshot.js`
  (run solo, per the standing rule) and visually verified —
  the enlarged 768×402 interior, "NINE FINE STOPS" header,
  Photo Booth island, Bounty Board and First Shift card.
  Synced to the repo-clone and handover packet copies. The
  packet now has no stale assets. Nothing was submitted
  anywhere.
- PACKET RE-SYNCED 2026-11-21 to v1.26 ("ROOT ACCESS" —
  the Source Room, live tuning, the Banzai World touch
  Start-menu fix, and the fine-grained source split):
  the AAB was verified ALREADY CURRENT — no restamp
  needed. `../TheSurvivalGame.aab` is 2,277,626 bytes
  (md5 c218226b32aaa716a5b91a6b424f5d96), byte-identical
  to the GitHub v1.26 release AAB in the handover
  folder, and its embedded `base/assets/index.html` is
  byte-identical (md5 99b6c4d36fb78df54557185d92800be4)
  to the live GitHub Pages build. Its manifest
  (bundletool dump) reads versionCode 26 / versionName
  "1.26", targetSdk 36, and declares ZERO
  uses-permission elements — FORMS.md's data-safety
  answers (no data collected, none shared) were
  re-verified against this AAB and stand unchanged.
  (The GitHub release APK is a different flavor — it
  declares INTERNET and targets SDK 34; the Play AAB is
  the build FORMS.md describes.) LISTING.md copy
  revised for the ten-stop Mall, the Source Room and
  live tuning in Android-accurate terms only
  (browse/edit/persist + live tuning — no rebuild
  claim; rebuilding is native-host-only) and the
  Banzai touch Start-menu fix, trimmed elsewhere to
  fit: full description re-counted at 3,994 of 4,000
  chars counting newlines (3,927 excluding them);
  title 17/30 and short description 64/80 unchanged.
  `assets/screenshot-03-mall.png` RECAPTURED
  2026-11-21 from the shipped v1.26 build (a
  playmallshot.js variant framed on the north wall,
  run solo, no hat feed): the Source Room's sealed
  door between THE COUNTER and WHALES ONLY, plaque
  and prompt line readable, visually verified — see
  LISTING.md. Synced to the repo-clone and handover
  packet copies. The packet now has no stale assets.
  Nothing was submitted anywhere.
- PACKET RESTAMPED 2026-11-23 to v1.29 ("ON THE BUTTON"
  — one restamp past v1.27 and v1.28, at Daniel's
  direction): `../TheSurvivalGame.aab` is now 2,418,790
  bytes (md5 c1f606279e928399c777ca5c4e5f5160),
  byte-identical to the GitHub v1.29 release AAB in the
  handover folder, bundletool-validated, versionCode 29
  / versionName "1.29", targetSdk 36, ZERO
  uses-permission elements, and its embedded
  `base/assets/index.html` is byte-identical
  (md5 f1638b546b92b6ddeef50bb5d7bfbc5b) to the live
  GitHub Pages build — FORMS.md's data-safety answers
  stand unchanged. Still debug-key signed — the
  dedicated upload keystore is still needed at
  submission. LISTING.md copy revised for v1.27 (Source
  Room touch editing in the copy; wilder WAR! fields,
  Tank Rescue pickups and podium-only Cart medals in the
  revision log), v1.28 (the eighth cabinet, MORDOR FUN
  RUN) and v1.29 (the fire-button swing, road powerups,
  WAR! skirmish save/resume) in Android-accurate terms
  only, trimmed elsewhere to fit: full description
  re-counted at 3,983 of 4,000 chars counting newlines
  (3,937 excluding them); title 17/30 and short
  description 64/80 unchanged.
  `assets/screenshot-08-mordor.png` ADDED 2026-11-23 —
  the eighth and last Play slot — captured from the
  shipped v1.29 build via `tests/playmordorshot.js`
  (run solo, visually verified): the runner on the
  Shire road at 1 of 1,000 moose with three goblins
  closing in real time. Synced to the goals-files and
  handover packet copies. Nothing was submitted
  anywhere.

## Daniel-only, in order

1. Free vs paid answer (recommended: free for release one).
2. New developer account: $25 + ID verification + Android device
   verification.
3. Recruit 14–15 testers.
